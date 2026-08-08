"""Serve an SP-050 CDN-identical `.spa` publish tree over HTTP (SP-051).

Document root = publish root so URLs match production::

    {base}/meta/maps.json
    {base}/maps/{MAP_SERIES}/{dataVersion}/countries.txt
    {base}/maps/{MAP_SERIES}/{dataVersion}/{leaf}.mwm|.spa

Supports HTTP Range (206) for chunked MWM downloads. Debug routes are opt-in.
"""

from __future__ import annotations

import argparse
import json
import logging
import mimetypes
import os
import socket
import time
from http.server import BaseHTTPRequestHandler
from http.server import ThreadingHTTPServer
from urllib.parse import unquote
from urllib.parse import urlparse


logger = logging.getLogger(__name__)

_BINARY_EXTS = {".mwm", ".spa", ".sig", ".diff", ".mwm.ready"}
_JSON_NAMES = {"countries.txt", "maps.json"}
_OPERATOR_ONLY_NAMES = frozenset({"inventory.json"})


def guess_content_type(path):
    name = os.path.basename(path)
    _, ext = os.path.splitext(name)
    if ext.lower() in _BINARY_EXTS:
        return "application/octet-stream"
    if name in _JSON_NAMES or name == "inventory.json" or ext.lower() == ".json":
        return "application/json"
    guessed, _ = mimetypes.guess_type(path)
    return guessed or "application/octet-stream"


def resolve_under_root(root, url_path, allow_operator_files=False):
    """Map a URL path to an absolute file under root, or None if unsafe/missing.

    Rejects path traversal and symlink escape outside root.
    Operator-only files (inventory.json) are hidden unless allow_operator_files.
    """
    if not url_path or url_path == "/":
        return None
    decoded = unquote(url_path)
    if "\x00" in decoded:
        return None
    rel = decoded.lstrip("/")
    if not rel:
        return None
    if not allow_operator_files:
        base = os.path.basename(rel)
        if base in _OPERATOR_ONLY_NAMES:
            return None
    root_real = os.path.realpath(root)
    candidate = os.path.realpath(os.path.join(root_real, rel))
    try:
        common = os.path.commonpath([root_real, candidate])
    except ValueError:
        return None
    if common != root_real:
        return None
    if not os.path.isfile(candidate):
        return None
    return candidate


def parse_range_header(range_header, file_size):
    """Parse a single-range ``bytes=`` header.

    Returns ``(start, end_inclusive)`` or ``None`` if the header is absent.
    Raises ``ValueError`` for malformed or unsatisfiable ranges (caller → 416).
    """
    if not range_header:
        return None
    header = range_header.strip()
    if not header.lower().startswith("bytes="):
        return None
    spec = header[6:].strip()
    if "," in spec:
        raise ValueError("multiple ranges not supported")
    if "-" not in spec:
        raise ValueError("malformed range")
    start_s, end_s = spec.split("-", 1)
    if start_s == "" and end_s == "":
        raise ValueError("empty range")
    if start_s == "":
        suffix = int(end_s)
        if suffix <= 0:
            raise ValueError("bad suffix")
        if file_size == 0:
            raise ValueError("empty file")
        start = max(0, file_size - suffix)
        end = file_size - 1
        return start, end
    start = int(start_s)
    if end_s == "":
        end = file_size - 1
    else:
        end = int(end_s)
    if start < 0 or end < start or start >= file_size:
        raise ValueError("unsatisfiable")
    end = min(end, file_size - 1)
    return start, end


def detect_lan_ipv4():
    """Best-effort LAN IPv4 for the Custom Maps URL banner."""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            sock.connect(("8.8.8.8", 80))
            return sock.getsockname()[0]
        finally:
            sock.close()
    except OSError:
        pass
    try:
        hostname = socket.gethostname()
        for info in socket.getaddrinfo(hostname, None, socket.AF_INET):
            ip = info[4][0]
            if not ip.startswith("127."):
                return ip
    except OSError:
        pass
    return "127.0.0.1"


def _count_inventory_spa_leaves(leaves):
    if isinstance(leaves, dict):
        return len(leaves)
    if not isinstance(leaves, list):
        return 0
    count = 0
    for entry in leaves:
        if isinstance(entry, str):
            count += 1
        elif isinstance(entry, dict):
            if entry.get("advertised", True) and (
                entry.get("spa_bytes") is not None or entry.get("id")
            ):
                count += 1
    return count


def load_health_payload(root):
    inventory_path = os.path.join(root, "inventory.json")
    map_series = None
    data_version = None
    spa_leaf_count = 0
    if os.path.isfile(inventory_path):
        try:
            with open(inventory_path, "r", encoding="utf-8") as f:
                inv = json.load(f)
            map_series = inv.get("map_series") or inv.get("map-series")
            data_version = inv.get("publish_version") or inv.get("data_version")
            leaves = inv.get("leaves") or inv.get("spa_leaves") or []
            spa_leaf_count = _count_inventory_spa_leaves(leaves)
        except (OSError, ValueError, TypeError) as exc:
            logger.warning("health: failed to read inventory.json: %s", exc)
    if map_series is None or data_version is None:
        maps_json = os.path.join(root, "meta", "maps.json")
        if os.path.isfile(maps_json):
            try:
                with open(maps_json, "r", encoding="utf-8") as f:
                    meta = json.load(f)
                series_map = meta.get("map-series") or {}
                if isinstance(series_map, dict) and series_map:
                    if map_series is None:
                        map_series = next(iter(series_map.keys()))
                    entry = series_map.get(map_series) or next(
                        iter(series_map.values())
                    )
                    if isinstance(entry, dict) and data_version is None:
                        data_version = entry.get("latest")
            except (OSError, ValueError, TypeError) as exc:
                logger.warning("health: failed to read meta/maps.json: %s", exc)
    if spa_leaf_count == 0 and map_series and data_version is not None:
        vdir = os.path.join(root, "maps", str(map_series), str(data_version))
        if os.path.isdir(vdir):
            spa_leaf_count = sum(
                1 for name in os.listdir(vdir) if name.endswith(".spa")
            )
    ok = bool(map_series is not None and data_version is not None)
    return {
        "ok": ok,
        "map_series": map_series,
        "data_version": data_version,
        "spa_leaf_count": spa_leaf_count,
    }


def warn_if_tree_incomplete(root):
    maps_json = os.path.join(root, "meta", "maps.json")
    if not os.path.isfile(maps_json):
        logger.warning(
            "publish root missing meta/maps.json — Custom Maps updates will fail"
        )
        print(
            "warning: missing meta/maps.json under {}".format(root),
            flush=True,
        )
        return
    maps_dir = os.path.join(root, "maps")
    if not os.path.isdir(maps_dir):
        logger.warning("publish root missing maps/ — leaf downloads will 404")
        print("warning: missing maps/ under {}".format(root), flush=True)


def make_handler(root, enable_debug_routes, log_access):
    root_abs = os.path.abspath(root)

    class SpaPublishHandler(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def log_message(self, fmt, *args):
            if log_access:
                logger.info("%s - %s", self.address_string(), fmt % args)

        def _access(self, method, path, status, nbytes, duration_ms):
            if log_access:
                logger.info(
                    "access method=%s path=%s status=%s bytes=%s duration_ms=%.1f",
                    method,
                    path,
                    status,
                    nbytes,
                    duration_ms,
                )

        def _send_not_found(self, path, started):
            self.send_error(404, "Not Found")
            self._access(
                self.command, path, 404, 0, (time.monotonic() - started) * 1000
            )

        def _handle_debug_inventory(self, path, started, head_only):
            if not enable_debug_routes:
                self._send_not_found(path, started)
                return True
            inv_path = os.path.join(root_abs, "inventory.json")
            if not os.path.isfile(inv_path):
                self.send_error(404, "inventory.json missing")
                self._access(
                    self.command, path, 404, 0, (time.monotonic() - started) * 1000
                )
                return True
            self._send_file(inv_path, path, started, head_only=head_only)
            return True

        def _handle_health(self, path, started, head_only):
            body = json.dumps(load_health_payload(root_abs)).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            if not head_only:
                self.wfile.write(body)
            self._access(
                self.command,
                path,
                200,
                0 if head_only else len(body),
                (time.monotonic() - started) * 1000,
            )

        def do_GET(self):
            self._dispatch(head_only=False)

        def do_HEAD(self):
            self._dispatch(head_only=True)

        def _dispatch(self, head_only):
            started = time.monotonic()
            parsed = urlparse(self.path)
            path = parsed.path or "/"

            if path in ("/health", "/healthz"):
                self._handle_health(path, started, head_only)
                return

            if path == "/debug/inventory":
                self._handle_debug_inventory(path, started, head_only)
                return

            if path in ("/", "/index.html"):
                self._send_not_found(path, started)
                return

            file_path = resolve_under_root(root_abs, path)
            if file_path is None:
                self._send_not_found(path, started)
                return
            self._send_file(file_path, path, started, head_only=head_only)

        def _send_file(self, file_path, url_path, started, head_only=False):
            file_size = os.path.getsize(file_path)
            content_type = guess_content_type(file_path)
            range_header = self.headers.get("Range")
            try:
                byte_range = parse_range_header(range_header, file_size)
            except ValueError:
                self.send_response(416)
                self.send_header("Content-Range", "bytes */{}".format(file_size))
                self.send_header("Content-Length", "0")
                self.end_headers()
                self._access(
                    self.command, url_path, 416, 0, (time.monotonic() - started) * 1000
                )
                return

            if byte_range is None:
                status = 200
                start = 0
                length = file_size
            else:
                status = 206
                start, end = byte_range
                length = end - start + 1

            self.send_response(status)
            self.send_header("Content-Type", content_type)
            self.send_header("Accept-Ranges", "bytes")
            self.send_header("Content-Length", str(length))
            if status == 206:
                self.send_header(
                    "Content-Range",
                    "bytes {}-{}/{}".format(start, start + length - 1, file_size),
                )
            self.send_header("Cache-Control", "no-transform")
            self.end_headers()

            nbytes = 0
            if not head_only and length > 0:
                try:
                    with open(file_path, "rb") as f:
                        f.seek(start)
                        remaining = length
                        while remaining > 0:
                            chunk = f.read(min(64 * 1024, remaining))
                            if not chunk:
                                break
                            self.wfile.write(chunk)
                            remaining -= len(chunk)
                            nbytes += len(chunk)
                except (BrokenPipeError, ConnectionResetError):
                    self._access(
                        self.command,
                        url_path,
                        status,
                        nbytes,
                        (time.monotonic() - started) * 1000,
                    )
                    return

            self._access(
                self.command,
                url_path,
                status,
                nbytes,
                (time.monotonic() - started) * 1000,
            )

    return SpaPublishHandler


def build_custom_maps_url(host, port, bind_host):
    display_host = host
    if bind_host in ("0.0.0.0", "::", ""):
        display_host = detect_lan_ipv4()
    elif bind_host == "127.0.0.1":
        display_host = "127.0.0.1"
    return "http://{}:{}/".format(display_host, port)


def serve_forever(root, host="0.0.0.0", port=8080, enable_debug_routes=False,
                  log_access=True):
    if not os.path.isdir(root):
        raise SystemExit("publish root not found: {}".format(root))
    warn_if_tree_incomplete(root)
    handler = make_handler(root, enable_debug_routes, log_access)
    server = ThreadingHTTPServer((host, port), handler)
    url = build_custom_maps_url(host, port, host)
    banner = [
        "SP-051 spa publish server",
        "  root: {}".format(os.path.abspath(root)),
        "  listen: {}:{}".format(host, port),
        "  Custom Maps URL (paste in app): {}".format(url),
        "  health: {}health".format(url),
        "  debug inventory: {}".format(
            "enabled at {}debug/inventory".format(url)
            if enable_debug_routes
            else "disabled (pass --enable-debug-routes)"
        ),
        "  adb reverse tip: adb reverse tcp:{0} tcp:{0} then use http://127.0.0.1:{0}/".format(
            port
        ),
    ]
    for line in banner:
        logger.info(line)
        print(line, flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down.", flush=True)
    finally:
        server.server_close()
    return 0


def build_arg_parser():
    parser = argparse.ArgumentParser(
        description="Serve an SP-050 spa publish tree for Custom Maps URL (SP-051)."
    )
    parser.add_argument(
        "--root",
        required=True,
        help="SP-050 --out publish root (contains meta/ and maps/)",
    )
    parser.add_argument("--host", default="0.0.0.0", help="Bind address (default 0.0.0.0)")
    parser.add_argument("--port", type=int, default=8080, help="Listen port (default 8080)")
    parser.add_argument(
        "--enable-debug-routes",
        action="store_true",
        help="Enable GET /debug/inventory (off by default)",
    )
    parser.add_argument(
        "--no-log-access",
        action="store_true",
        help="Disable per-request access logs",
    )
    parser.add_argument(
        "--print-url-only",
        action="store_true",
        help="Print suggested Custom Maps URL and exit (no listen)",
    )
    return parser


def main(argv=None):
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
    )
    args = build_arg_parser().parse_args(argv)
    root = os.path.abspath(args.root)
    if args.print_url_only:
        print(build_custom_maps_url(args.host, args.port, args.host))
        return 0
    return serve_forever(
        root=root,
        host=args.host,
        port=args.port,
        enable_debug_routes=args.enable_debug_routes,
        log_access=not args.no_log_access,
    )


if __name__ == "__main__":
    raise SystemExit(main())
