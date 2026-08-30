"""Street Pixels map-signing identity and World bootstrap (SP-101)."""

from __future__ import annotations

import argparse
import ipaddress
import json
import os
import shutil
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request

from street_pixels.map_pipeline import denied_host_in
from street_pixels.map_pipeline import url_hostname


_MODULE_DIR = os.path.dirname(os.path.abspath(__file__))
_TOOLS_PYTHON = os.path.dirname(_MODULE_DIR)
_REPO_ROOT = os.path.dirname(os.path.dirname(_TOOLS_PYTHON))

EXAMPLE_PRIVATE_H = os.path.join(_REPO_ROOT, "private.h.street-pixels.example")
PLACEHOLDER_MAP_ORIGIN = "https://maps.example.invalid/"
LOCKED_MAP_SERIES = "2026.06.28"

ACTION_SKIP = "skip"
ACTION_KEEP = "keep"
ACTION_COPY = "copy"
ACTION_FETCH = "fetch"

WORLD_NAME = "World.mwm"
WORLD_COASTS_NAME = "WorldCoasts.mwm"

_PRIVATE_NETWORKS = (
    ipaddress.ip_network("10.0.0.0/8"),
    ipaddress.ip_network("172.16.0.0/12"),
    ipaddress.ip_network("192.168.0.0/16"),
    ipaddress.ip_network("127.0.0.0/8"),
    ipaddress.ip_network("169.254.0.0/16"),
    ipaddress.ip_network("::1/128"),
    ipaddress.ip_network("fc00::/7"),
    ipaddress.ip_network("fe80::/10"),
)

_MISSING_BOOTSTRAP = (
    "Street Pixels will not download World.mwm from CoMaps map hosts "
    "(SPD-087). Set SKIP_MAP_DOWNLOAD=1, or STREET_PIXELS_LOCAL_WORLD to a "
    "World.mwm file, or STREET_PIXELS_WORLD_DIR to a directory containing "
    "World.mwm, or STREET_PIXELS_MAPS_BASE_URL to a non-CoMaps HTTPS origin "
    "(SP-102 fills the public host). Do not use mapgen-fi-1.comaps.app or "
    "other CoMaps map CDNs."
)


class MapIdentityError(Exception):
    """Fail-closed map identity / World bootstrap error."""


def is_nonempty(value):
    return value is not None and str(value).strip() != ""


def host_is_private(host):
    if not host:
        return False
    text = str(host).strip().strip("[]").lower()
    if text == "localhost":
        return True
    try:
        addr = ipaddress.ip_address(text)
    except ValueError:
        return False
    return any(addr in net for net in _PRIVATE_NETWORKS)


def origin_is_private(url):
    return host_is_private(url_hostname(url))


def comaps_map_host_in(value):
    return denied_host_in(value)


def require_https_maps_origin(url):
    if not is_nonempty(url):
        raise MapIdentityError("maps base URL is empty")
    text = str(url).strip()
    parsed = urllib.parse.urlparse(text)
    if parsed.scheme != "https":
        raise MapIdentityError(
            "STREET_PIXELS_MAPS_BASE_URL must be HTTPS, not {!r} (SP-004)".format(
                parsed.scheme or text
            )
        )
    host = (parsed.hostname or "").lower()
    if not host:
        raise MapIdentityError("STREET_PIXELS_MAPS_BASE_URL has no host")
    if host_is_private(host):
        raise MapIdentityError(
            "STREET_PIXELS_MAPS_BASE_URL must not be a private-range host "
            "({!r}); use STREET_PIXELS_LOCAL_WORLD or STREET_PIXELS_WORLD_DIR "
            "(SP-004 / D12)".format(host)
        )
    denied = comaps_map_host_in(text)
    if denied:
        raise MapIdentityError(
            "refusing CoMaps map host {!r} in STREET_PIXELS_MAPS_BASE_URL "
            "(SPD-087)".format(denied)
        )
    return text.rstrip("/") + "/"


def join_world_url(base_url, map_series, mwm_version, filename):
    origin = require_https_maps_origin(base_url)
    rel = "maps/{}/{}/{}".format(map_series, mwm_version, filename)
    return urllib.parse.urljoin(origin, rel)


def read_map_series_and_version(countries_path):
    with open(countries_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    series = data.get("map_series")
    version = data.get("v")
    if not series or version is None:
        raise MapIdentityError(
            "countries file {!r} is missing map_series or v".format(countries_path)
        )
    return str(series), str(version)


def versioned_map_path(data_dir, mwm_version, filename):
    return os.path.join(data_dir, "world_mwm", str(mwm_version), filename)


def world_file_present(data_dir, mwm_version):
    versioned = versioned_map_path(data_dir, mwm_version, WORLD_NAME)
    link = os.path.join(data_dir, WORLD_NAME)
    return os.path.isfile(versioned) or os.path.isfile(link)


def _first_existing_file(paths):
    for path in paths:
        if path and os.path.isfile(path):
            return path
    return None


def resolve_local_world_files(local_world, world_dir, mwm_version):
    world = None
    coasts = None
    if is_nonempty(local_world):
        path = os.path.abspath(str(local_world).strip())
        if not os.path.isfile(path):
            raise MapIdentityError(
                "STREET_PIXELS_LOCAL_WORLD is not a file: {}".format(path)
            )
        world = path
    if is_nonempty(world_dir):
        directory = os.path.abspath(str(world_dir).strip())
        if not os.path.isdir(directory):
            raise MapIdentityError(
                "STREET_PIXELS_WORLD_DIR is not a directory: {}".format(directory)
            )
        if world is None:
            world = _first_existing_file(
                [
                    os.path.join(directory, WORLD_NAME),
                    os.path.join(directory, "world_mwm", str(mwm_version), WORLD_NAME),
                ]
            )
            if world is None:
                raise MapIdentityError(
                    "STREET_PIXELS_WORLD_DIR has no World.mwm: {}".format(directory)
                )
        coasts = _first_existing_file(
            [
                os.path.join(directory, WORLD_COASTS_NAME),
                os.path.join(
                    directory, "world_mwm", str(mwm_version), WORLD_COASTS_NAME
                ),
            ]
        )
        if world and os.path.dirname(world) and coasts is None:
            sibling = os.path.join(os.path.dirname(world), WORLD_COASTS_NAME)
            coasts = _first_existing_file([sibling])
    elif world is not None:
        sibling = os.path.join(os.path.dirname(world), WORLD_COASTS_NAME)
        coasts = _first_existing_file([sibling])
    return world, coasts


def _chosen_fetch_url(maps_base_url, legacy_maps_base_url):
    if is_nonempty(maps_base_url):
        return str(maps_base_url).strip()
    if is_nonempty(legacy_maps_base_url):
        return str(legacy_maps_base_url).strip()
    return ""


def resolve_world_bootstrap(
    skip_map_download="",
    maps_base_url="",
    legacy_maps_base_url="",
    local_world="",
    world_dir="",
    data_dir="",
    map_series="",
    mwm_version="",
):
    if is_nonempty(skip_map_download):
        return {
            "action": ACTION_SKIP,
            "world_source": None,
            "coasts_source": None,
            "world_url": None,
            "coasts_url": None,
            "map_series": map_series,
            "mwm_version": mwm_version,
        }

    if not map_series or not mwm_version:
        raise MapIdentityError("map_series and mwm_version are required")

    if data_dir and world_file_present(data_dir, mwm_version):
        return {
            "action": ACTION_KEEP,
            "world_source": None,
            "coasts_source": None,
            "world_url": None,
            "coasts_url": None,
            "map_series": map_series,
            "mwm_version": mwm_version,
        }

    if is_nonempty(local_world) or is_nonempty(world_dir):
        world, coasts = resolve_local_world_files(local_world, world_dir, mwm_version)
        return {
            "action": ACTION_COPY,
            "world_source": world,
            "coasts_source": coasts,
            "world_url": None,
            "coasts_url": None,
            "map_series": map_series,
            "mwm_version": mwm_version,
        }

    chosen = _chosen_fetch_url(maps_base_url, legacy_maps_base_url)
    if chosen:
        denied = comaps_map_host_in(chosen)
        if denied:
            raise MapIdentityError(
                "refusing CoMaps map host {!r} as World download origin "
                "(SPD-087). {}".format(denied, _MISSING_BOOTSTRAP)
            )
        origin = require_https_maps_origin(chosen)
        return {
            "action": ACTION_FETCH,
            "world_source": None,
            "coasts_source": None,
            "world_url": join_world_url(origin, map_series, mwm_version, WORLD_NAME),
            "coasts_url": join_world_url(
                origin, map_series, mwm_version, WORLD_COASTS_NAME
            ),
            "map_series": map_series,
            "mwm_version": mwm_version,
        }

    raise MapIdentityError(_MISSING_BOOTSTRAP)


def download_to_file(url, dest):
    denied = comaps_map_host_in(url)
    if denied:
        raise MapIdentityError(
            "refusing to download from CoMaps map host {!r}".format(denied)
        )
    if origin_is_private(url):
        raise MapIdentityError(
            "refusing to download World from private-range host (SP-004)"
        )
    tmp = dest + ".tmp"
    parent = os.path.dirname(dest)
    if parent:
        os.makedirs(parent, exist_ok=True)
    try:
        urllib.request.urlretrieve(url, tmp)
    except urllib.error.HTTPError as exc:
        if os.path.exists(tmp):
            os.remove(tmp)
        if exc.code == 404:
            return False
        raise MapIdentityError(
            "download failed HTTP {} for {}".format(exc.code, url)
        ) from exc
    except urllib.error.URLError as exc:
        if os.path.exists(tmp):
            os.remove(tmp)
        raise MapIdentityError(
            "download failed for {}: {}".format(url, exc.reason)
        ) from exc
    os.replace(tmp, dest)
    return True


def _link_in_data_dir(data_dir, versioned_path, link_name):
    link = os.path.join(data_dir, link_name)
    rel = os.path.relpath(versioned_path, data_dir)
    if os.path.lexists(link):
        os.remove(link)
    os.symlink(rel, link)


def _install_world_file(source, dest, data_dir, link_name):
    parent = os.path.dirname(dest)
    os.makedirs(parent, exist_ok=True)
    if os.path.abspath(source) != os.path.abspath(dest):
        shutil.copy2(source, dest)
    _link_in_data_dir(data_dir, dest, link_name)


def apply_world_bootstrap(plan, data_dir, downloader=None):
    if downloader is None:
        downloader = download_to_file
    action = plan["action"]
    mwm_version = plan["mwm_version"]
    world_dest = versioned_map_path(data_dir, mwm_version, WORLD_NAME)
    coasts_dest = versioned_map_path(data_dir, mwm_version, WORLD_COASTS_NAME)

    if action == ACTION_SKIP:
        print("Skipping world map download...")
        return plan

    if action == ACTION_KEEP:
        if os.path.isfile(world_dest):
            _link_in_data_dir(data_dir, world_dest, WORLD_NAME)
        elif os.path.isfile(os.path.join(data_dir, WORLD_NAME)):
            pass
        if os.path.isfile(coasts_dest):
            _link_in_data_dir(data_dir, coasts_dest, WORLD_COASTS_NAME)
        print("Using existing World.mwm (no download).")
        return plan

    if action == ACTION_COPY:
        print("Copying world map from operator-provided local file...")
        _install_world_file(plan["world_source"], world_dest, data_dir, WORLD_NAME)
        if plan["coasts_source"]:
            _install_world_file(
                plan["coasts_source"], coasts_dest, data_dir, WORLD_COASTS_NAME
            )
        else:
            print(
                "WorldCoasts.mwm not found beside local World; omitting (SPD-094)."
            )
        return plan

    if action == ACTION_FETCH:
        world_url = plan["world_url"]
        coasts_url = plan["coasts_url"]
        denied = comaps_map_host_in(world_url) or comaps_map_host_in(coasts_url)
        if denied:
            raise MapIdentityError(
                "refusing to fetch World from CoMaps map host {!r}".format(denied)
            )
        print("Downloading world map from STREET_PIXELS_MAPS_BASE_URL...")
        os.makedirs(os.path.dirname(world_dest), exist_ok=True)
        if not downloader(world_url, world_dest):
            raise MapIdentityError(
                "could not download World.mwm from STREET_PIXELS origin "
                "(HTTP 404). {}".format(_MISSING_BOOTSTRAP)
            )
        _link_in_data_dir(data_dir, world_dest, WORLD_NAME)
        coasts_ok = downloader(coasts_url, coasts_dest)
        if coasts_ok:
            _link_in_data_dir(data_dir, coasts_dest, WORLD_COASTS_NAME)
        else:
            if os.path.exists(coasts_dest):
                os.remove(coasts_dest)
            print(
                "WorldCoasts.mwm not available from STREET_PIXELS origin "
                "(HTTP 404); omitting (SPD-094)."
            )
        return plan

    raise MapIdentityError("unknown World bootstrap action {!r}".format(action))


def configure_world(
    data_dir,
    countries_path,
    skip_map_download="",
    maps_base_url="",
    legacy_maps_base_url="",
    local_world="",
    world_dir="",
    downloader=None,
):
    map_series = ""
    mwm_version = ""
    if not is_nonempty(skip_map_download):
        map_series, mwm_version = read_map_series_and_version(countries_path)
    plan = resolve_world_bootstrap(
        skip_map_download=skip_map_download,
        maps_base_url=maps_base_url,
        legacy_maps_base_url=legacy_maps_base_url,
        local_world=local_world,
        world_dir=world_dir,
        data_dir=data_dir,
        map_series=map_series,
        mwm_version=mwm_version,
    )
    return apply_world_bootstrap(plan, data_dir, downloader=downloader)


def generate_ed25519_pem_pair(secret_path, public_path):
    proc = subprocess.run(
        ["openssl", "genpkey", "-algorithm", "Ed25519", "-out", secret_path],
        capture_output=True,
        text=True,
        check=False,
    )
    if proc.returncode != 0:
        raise MapIdentityError(
            "openssl genpkey Ed25519 failed: {}".format(proc.stderr.strip())
        )
    proc = subprocess.run(
        ["openssl", "pkey", "-in", secret_path, "-pubout", "-out", public_path],
        capture_output=True,
        text=True,
        check=False,
    )
    if proc.returncode != 0:
        raise MapIdentityError(
            "openssl pkey -pubout failed: {}".format(proc.stderr.strip())
        )


def ed25519_public_key_hex(public_pem_path):
    proc = subprocess.run(
        ["openssl", "pkey", "-pubin", "-in", public_pem_path, "-outform", "DER"],
        capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        err = proc.stderr.decode("utf-8", "replace").strip()
        raise MapIdentityError("openssl pkey DER export failed: {}".format(err))
    der = proc.stdout
    if len(der) < 32:
        raise MapIdentityError("Ed25519 DER public key is shorter than 32 bytes")
    return der[-32:].hex()


def sign_rawin(file_path, secret_pem, signature_path=None):
    if signature_path is None:
        signature_path = file_path + ".sig"
    cmd = [
        "openssl",
        "pkeyutl",
        "-sign",
        "-inkey",
        secret_pem,
        "-rawin",
        "-in",
        file_path,
        "-out",
        signature_path,
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if proc.returncode != 0:
        raise MapIdentityError(
            "openssl pkeyutl -sign -rawin failed (exit {} stdout {!r} stderr {!r})".format(
                proc.returncode, proc.stdout.strip(), proc.stderr.strip()
            )
        )
    return signature_path


def verify_rawin(file_path, signature_path, public_pem):
    cmd = [
        "openssl",
        "pkeyutl",
        "-verify",
        "-pubin",
        "-inkey",
        public_pem,
        "-rawin",
        "-in",
        file_path,
        "-sigfile",
        signature_path,
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    return proc.returncode == 0


def _cmd_configure_world(args):
    configure_world(
        data_dir=args.data_dir,
        countries_path=args.countries,
        skip_map_download=args.skip_map_download,
        maps_base_url=args.maps_base_url,
        legacy_maps_base_url=args.legacy_maps_base_url,
        local_world=args.local_world,
        world_dir=args.world_dir,
    )
    return 0


def _cmd_public_hex(args):
    print(ed25519_public_key_hex(args.public_key))
    return 0


def _cmd_resolve(args):
    map_series = args.map_series
    mwm_version = args.mwm_version
    if args.countries and (not map_series or not mwm_version):
        map_series, mwm_version = read_map_series_and_version(args.countries)
    plan = resolve_world_bootstrap(
        skip_map_download=args.skip_map_download,
        maps_base_url=args.maps_base_url,
        legacy_maps_base_url=args.legacy_maps_base_url,
        local_world=args.local_world,
        world_dir=args.world_dir,
        data_dir=args.data_dir,
        map_series=map_series,
        mwm_version=mwm_version,
    )
    json.dump(plan, sys.stdout)
    sys.stdout.write("\n")
    return 0


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    parser = argparse.ArgumentParser(
        description="Street Pixels map identity helpers (SP-101)"
    )
    sub = parser.add_subparsers(dest="command")

    conf = sub.add_parser(
        "configure-world",
        help="Provision data/World.mwm without CoMaps map hosts",
    )
    conf.add_argument("--data-dir", required=True)
    conf.add_argument("--countries", required=True)
    conf.add_argument("--skip-map-download", default="")
    conf.add_argument("--maps-base-url", default="")
    conf.add_argument("--legacy-maps-base-url", default="")
    conf.add_argument("--local-world", default="")
    conf.add_argument("--world-dir", default="")
    conf.set_defaults(handler=_cmd_configure_world)

    hx = sub.add_parser(
        "public-hex",
        help="Print 64-char Ed25519 public key hex from a public PEM",
    )
    hx.add_argument("--public-key", required=True)
    hx.set_defaults(handler=_cmd_public_hex)

    nxt = sub.add_parser("resolve", help="Print World bootstrap plan as JSON")
    nxt.add_argument("--data-dir", default="")
    nxt.add_argument("--countries", default="")
    nxt.add_argument("--map-series", default="")
    nxt.add_argument("--mwm-version", default="")
    nxt.add_argument("--skip-map-download", default="")
    nxt.add_argument("--maps-base-url", default="")
    nxt.add_argument("--legacy-maps-base-url", default="")
    nxt.add_argument("--local-world", default="")
    nxt.add_argument("--world-dir", default="")
    nxt.set_defaults(handler=_cmd_resolve)

    args = parser.parse_args(argv)
    if not getattr(args, "command", None):
        parser.print_help()
        return 2
    try:
        return args.handler(args)
    except MapIdentityError as exc:
        print("ERROR: {}".format(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
