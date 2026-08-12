"""Prepare a CDN-identical spa publish root for LAN debugging (pre-mapgen).

Assumes the device is on the public CDN latest for MAP_SERIES: fetches
meta/maps.json → countries.txt from public mirrors, injects spa meta from
--spa-dir, and runs assemble_spa_publish_tree (spa-only by default).

Does not merge spa ads into git data/countries.txt. Does not bypass Ed25519.
"""

from __future__ import annotations

import argparse
import json
import logging
import os
import sys
import urllib.error
import urllib.request
from urllib.parse import quote
from urllib.parse import urljoin

from post_generation.assemble_spa_publish_tree import AssembleError
from post_generation.assemble_spa_publish_tree import assemble_spa_publish_tree
from post_generation.assemble_spa_publish_tree import try_read_spa_map_data_version
from post_generation.inject_spa_meta import inject_spa_meta


logger = logging.getLogger(__name__)

DEFAULT_MAP_SERIES = "2026.06.28"
DEFAULT_OUT = "/tmp/spa_debug_root"
DEFAULT_HELSINKI_LEAF = "Finland_Southern Finland_Helsinki"

# Matches private.h DEFAULT_URLS_JSON (+ meta-friendly hosts). Order = try order.
DEFAULT_CDN_BASES = (
    "https://mapgen-fi-1.comaps.app/",
    "https://cdn-fi-1.comaps.app/",
    "https://comaps.firewall-gateway.de/",
    "https://cdn-us-2.comaps.tech/",
    "https://comaps.openstreetmap.fr/",
    "https://comaps-it1.unfoxo.it/",
    "https://comaps-cdn.s3-website.cloud.ru/",
    "https://cdn.comaps.app/",
    "https://cdn-us-1.comaps.app/",
)

META_PREFERRED_BASES = (
    "https://cdn.comaps.app/",
    "https://cdn-us-1.comaps.app/",
) + DEFAULT_CDN_BASES


class PrepareError(Exception):
    """Operator-facing fail-closed error."""


def _normalize_base(base):
    base = (base or "").strip()
    if not base:
        raise PrepareError("empty CDN base URL")
    if not base.endswith("/"):
        base += "/"
    return base


def _http_get(url, timeout_s=60):
    req = urllib.request.Request(
        url,
        headers={"User-Agent": "street-pixels-prepare-spa-debug-root/1.0"},
    )
    with urllib.request.urlopen(req, timeout=timeout_s) as resp:
        status = getattr(resp, "status", None) or resp.getcode()
        if status != 200:
            raise PrepareError("HTTP {} for {}".format(status, url))
        return resp.read()


def _http_get_first(bases, relative_path, timeout_s=60):
    """Try each CDN base until a 200 body is returned. Returns (bytes, base_used)."""
    errors = []
    for base in bases:
        base = _normalize_base(base)
        url = urljoin(base, relative_path)
        try:
            body = _http_get(url, timeout_s=timeout_s)
            if not body:
                errors.append("{}: empty body".format(url))
                continue
            return body, base
        except (PrepareError, urllib.error.URLError, urllib.error.HTTPError, TimeoutError, OSError) as exc:
            errors.append("{}: {}".format(url, exc))
            logger.debug("CDN miss: %s", errors[-1])
    raise PrepareError(
        "failed to download {!r} from any CDN base:\n  {}".format(
            relative_path, "\n  ".join(errors[:8])
        )
    )


def fetch_latest_data_version(map_series, cdn_bases=None, timeout_s=60):
    bases = tuple(cdn_bases) if cdn_bases else META_PREFERRED_BASES
    body, base = _http_get_first(bases, "meta/maps.json", timeout_s=timeout_s)
    try:
        meta = json.loads(body.decode("utf-8"))
    except (UnicodeDecodeError, ValueError) as exc:
        raise PrepareError("meta/maps.json is not JSON: {}".format(exc)) from exc
    series_map = meta.get("map-series") or {}
    entry = series_map.get(str(map_series))
    if not isinstance(entry, dict):
        raise PrepareError(
            "meta/maps.json has no map-series entry for {!r} (from {})".format(
                map_series, base
            )
        )
    latest = entry.get("latest")
    if latest is None:
        raise PrepareError(
            "meta/maps.json entry for {!r} missing latest (from {})".format(
                map_series, base
            )
        )
    status = entry.get("status")
    logger.info(
        "CDN latest for %s = %s (status=%s) via %s",
        map_series,
        latest,
        status,
        base,
    )
    return int(latest), base, meta


def countries_relative_url(map_series, data_version):
    return "maps/{}/{}/countries.txt".format(map_series, int(data_version))


def fetch_countries_txt(map_series, data_version, cdn_bases=None, timeout_s=120):
    bases = tuple(cdn_bases) if cdn_bases else DEFAULT_CDN_BASES
    rel = countries_relative_url(map_series, data_version)
    body, base = _http_get_first(bases, rel, timeout_s=timeout_s)
    try:
        countries = json.loads(body.decode("utf-8"))
    except (UnicodeDecodeError, ValueError) as exc:
        raise PrepareError("countries.txt is not JSON: {}".format(exc)) from exc
    if int(countries.get("v")) != int(data_version):
        raise PrepareError(
            "downloaded countries \"v\" {} != expected {}".format(
                countries.get("v"), data_version
            )
        )
    if countries.get("map_series") != map_series:
        raise PrepareError(
            "downloaded countries map_series {!r} != {!r}".format(
                countries.get("map_series"), map_series
            )
        )
    logger.info("Downloaded countries.txt (%s bytes) via %s", len(body), base)
    return body, countries, base


def _warn_spa_header_versions(spa_dir, leaf_ids, data_version):
    for leaf_id in leaf_ids:
        path = os.path.join(spa_dir, "{}.spa".format(leaf_id))
        if not os.path.isfile(path):
            continue
        hdr = try_read_spa_map_data_version(path)
        if hdr is None:
            continue
        if int(hdr) != int(data_version):
            logger.warning(
                "spa header map_data_version %s for %s != data-version %s "
                "(areas may fail-closed VersionMismatch on device)",
                hdr,
                leaf_id,
                data_version,
            )


def _list_spa_leaves(spa_dir, leaves_arg):
    if leaves_arg:
        return [x.strip() for x in leaves_arg.split(",") if x.strip()]
    names = []
    for name in sorted(os.listdir(spa_dir)):
        if name.endswith(".spa") and os.path.isfile(os.path.join(spa_dir, name)):
            names.append(name[: -len(".spa")])
    return names


def prepare_spa_debug_root(
    spa_dir,
    out=DEFAULT_OUT,
    map_series=DEFAULT_MAP_SERIES,
    data_version=None,
    leaves=None,
    channel="serve-only",
    publish_version=None,
    secret_key=None,
    mwm_dir=None,
    spa_only=True,
    cdn_bases=None,
    countries_path=None,
    skip_cdn=False,
    dry_run=False,
    check_spa_version=True,
):
    """Build publish root. Returns a result dict for printing / tests."""
    if not os.path.isdir(spa_dir):
        raise PrepareError("spa directory not found: {}".format(spa_dir))

    channel = (channel or "serve-only").lower()
    if channel not in ("serve-only", "a", "b"):
        raise PrepareError("unknown --channel {!r} (serve-only|A|B)".format(channel))

    if channel == "a":
        if not secret_key:
            raise PrepareError("Channel A requires --secret-key")
        if publish_version is None and data_version is not None:
            publish_version = int(data_version) + 1
        elif publish_version is None:
            pass  # filled after CDN latest known

    cdn_bases_list = None
    if cdn_bases:
        cdn_bases_list = [_normalize_base(b) for b in cdn_bases]

    meta_base = None
    countries_base = None
    countries_bytes = None

    if countries_path:
        if not os.path.isfile(countries_path):
            raise PrepareError("countries file not found: {}".format(countries_path))
        with open(countries_path, "rb") as f:
            countries_bytes = f.read()
        countries = json.loads(countries_bytes.decode("utf-8"))
        data_version = int(countries["v"])
        map_series = countries.get("map_series") or map_series
        logger.info("Using local countries %s (v=%s)", countries_path, data_version)
    else:
        if skip_cdn:
            raise PrepareError("no --countries and --skip-cdn set")
        latest, meta_base, _meta = fetch_latest_data_version(
            map_series, cdn_bases=cdn_bases_list
        )
        if data_version is None:
            data_version = latest
        elif int(data_version) != int(latest):
            logger.warning(
                "requested --data-version %s differs from CDN latest %s for %s; "
                "using requested version",
                data_version,
                latest,
                map_series,
            )
        countries_bytes, countries, countries_base = fetch_countries_txt(
            map_series, data_version, cdn_bases=cdn_bases_list
        )

    if channel == "a" and publish_version is None:
        publish_version = int(data_version) + 1

    leaf_ids = _list_spa_leaves(spa_dir, leaves)
    if not leaf_ids:
        raise PrepareError("no .spa files under {}".format(spa_dir))
    if leaves is None and DEFAULT_HELSINKI_LEAF in leaf_ids:
        # Prefer Helsinki-only when present and caller did not pass --leaves.
        leaf_ids = [DEFAULT_HELSINKI_LEAF]
    leaves_csv = ",".join(leaf_ids)

    if check_spa_version:
        _warn_spa_header_versions(spa_dir, leaf_ids, data_version)

    out_abs = os.path.abspath(out)
    cache_dir = os.path.join(out_abs, "_cdn_cache")
    os.makedirs(cache_dir, exist_ok=True)
    cached_countries = os.path.join(
        cache_dir, "countries_{}_{}.txt".format(map_series, data_version)
    )
    with open(cached_countries, "wb") as f:
        f.write(countries_bytes)

    channel_b_countries = None
    if channel == "b":
        channel_b_dir = os.path.join(out_abs, "_channel_b")
        os.makedirs(channel_b_dir, exist_ok=True)
        channel_b_countries = os.path.join(channel_b_dir, "countries.txt")
        patched = json.loads(countries_bytes.decode("utf-8"))
        inject_spa_meta(patched, spa_dir)
        allow = set(leaf_ids)

        def _strip_non_allowlisted(node):
            if "g" in node:
                for child in node["g"]:
                    _strip_non_allowlisted(child)
            else:
                leaf_id = node.get("id")
                if leaf_id not in allow:
                    node.pop("spa", None)
                    node.pop("spa_sha1_base64", None)

        _strip_non_allowlisted(patched)
        with open(channel_b_countries, "w", encoding="utf-8") as f:
            json.dump(patched, f, ensure_ascii=False, indent=1)
            f.write("\n")
        logger.info(
            "Channel B: wrote injected countries to %s (do not merge to git)",
            channel_b_countries,
        )

    effective_publish = int(publish_version) if publish_version is not None else int(data_version)

    if dry_run:
        return {
            "dry_run": True,
            "out": out_abs,
            "map_series": map_series,
            "data_version": int(data_version),
            "publish_version": effective_publish,
            "leaves": leaf_ids,
            "channel": channel,
            "countries_cache": cached_countries,
            "channel_b_countries": channel_b_countries,
            "meta_base": meta_base,
            "countries_base": countries_base,
        }

    rc = assemble_spa_publish_tree(
        countries_path=cached_countries,
        spa_dir=spa_dir,
        out=out_abs,
        map_series=map_series,
        data_version=int(data_version),
        publish_version=effective_publish if channel == "a" else int(data_version),
        leaves=leaves_csv,
        secret_key=secret_key if channel == "a" else None,
        include_mwm=not spa_only,
        mwm_dir=mwm_dir,
        spa_only=spa_only,
    )
    if rc != 0:
        raise PrepareError("assemble_spa_publish_tree failed with code {}".format(rc))

    # Encode leaf for curl tip (space → %20).
    sample_leaf = leaf_ids[0]
    sample_url_path = "maps/{}/{}/{}".format(
        map_series,
        effective_publish if channel == "a" else int(data_version),
        quote("{}.spa".format(sample_leaf), safe=""),
    )

    return {
        "dry_run": False,
        "out": out_abs,
        "map_series": map_series,
        "data_version": int(data_version),
        "publish_version": effective_publish if channel == "a" else int(data_version),
        "leaves": leaf_ids,
        "channel": channel,
        "countries_cache": cached_countries,
        "channel_b_countries": channel_b_countries,
        "meta_base": meta_base,
        "countries_base": countries_base,
        "sample_url_path": sample_url_path,
    }


def print_result(result, port=8080):
    print("OK: {}".format(result["out"]))
    print("  map_series={}".format(result["map_series"]))
    print("  data_version={}".format(result["data_version"]))
    print("  publish_version={}".format(result["publish_version"]))
    print("  channel={}".format(result["channel"]))
    print("  leaves={}".format(", ".join(result["leaves"])))
    if result.get("meta_base"):
        print("  meta_from={}".format(result["meta_base"]))
    if result.get("countries_base"):
        print("  countries_from={}".format(result["countries_base"]))
    print("  countries_cache={}".format(result["countries_cache"]))
    if result.get("dry_run"):
        print("  (dry-run — nothing assembled)")
        return
    if result.get("channel_b_countries"):
        print("  channel_b_countries={}".format(result["channel_b_countries"]))
        print("  Channel B: embed that countries file in a local APK only — do not merge to git.")
    if result["channel"] == "a":
        print(
            "  Channel A: device must Check for updates to pick up publish_version {}.".format(
                result["publish_version"]
            )
        )
    print("Next:")
    print(
        "  cd tools/python && PYTHONPATH=. python3 -m street_pixels serve_spa_publish_tree \\"
    )
    print(
        "    --root {} --host 0.0.0.0 --port {}".format(result["out"], port)
    )
    print("  curl -s http://127.0.0.1:{}/health".format(port))
    if result.get("sample_url_path"):
        print(
            "  curl -sI http://127.0.0.1:{}/{}".format(
                port, result["sample_url_path"]
            )
        )
    print("  App → Settings → Advanced → Custom Maps server → http://<lan-ip>:{}/".format(port))
    print("  adb reverse tip: adb reverse tcp:{0} tcp:{0}".format(port))


def build_arg_parser():
    p = argparse.ArgumentParser(
        description=(
            "Prepare CDN-identical spa publish root for LAN debug "
            "(downloads latest countries.txt from public CDN)."
        )
    )
    p.add_argument("--spa-dir", required=True, help="Directory of {leaf}.spa")
    p.add_argument("--out", default=DEFAULT_OUT, help="Publish root (default {})".format(DEFAULT_OUT))
    p.add_argument(
        "--map-series",
        default=DEFAULT_MAP_SERIES,
        help="MAP_SERIES (default {})".format(DEFAULT_MAP_SERIES),
    )
    p.add_argument(
        "--data-version",
        type=int,
        default=None,
        help="Override CDN latest (default: meta/maps.json latest for map-series)",
    )
    p.add_argument(
        "--leaves",
        default=None,
        help="Comma-separated leaf ids (default: Helsinki if present in spa-dir, else all)",
    )
    p.add_argument(
        "--channel",
        default="serve-only",
        choices=["serve-only", "A", "B", "a", "b"],
        help="serve-only (default) | A (signed bump) | B (local inject copy)",
    )
    p.add_argument("--publish-version", type=int, default=None, help="Channel A bump (default: data-version+1)")
    p.add_argument("--secret-key", default=None, help="Channel A Ed25519 PEM")
    p.add_argument("--mwm-dir", default=None, help="Required unless --spa-only")
    p.add_argument(
        "--spa-only",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Do not copy MWMs (default true — phone already has maps from CDN)",
    )
    p.add_argument(
        "--cdn-base",
        action="append",
        default=None,
        help="CDN base URL (repeatable). Default: private.h DEFAULT_URLS mirrors",
    )
    p.add_argument(
        "--countries",
        default=None,
        help="Use local countries.txt instead of downloading from CDN",
    )
    p.add_argument("--skip-cdn", action="store_true", help="Require --countries; do not hit network")
    p.add_argument("--dry-run", action="store_true", help="Resolve CDN + plan only")
    p.add_argument(
        "--no-check-spa-version",
        action="store_true",
        help="Skip spa header vs data-version warning",
    )
    p.add_argument("--port", type=int, default=8080, help="Port printed in serve tips")
    p.add_argument("--verbose", action="store_true")
    return p


def main(argv=None):
    args = build_arg_parser().parse_args(argv)
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(levelname)s %(message)s",
    )
    try:
        result = prepare_spa_debug_root(
            spa_dir=args.spa_dir,
            out=args.out,
            map_series=args.map_series,
            data_version=args.data_version,
            leaves=args.leaves,
            channel=args.channel.lower(),
            publish_version=args.publish_version,
            secret_key=args.secret_key,
            mwm_dir=args.mwm_dir,
            spa_only=args.spa_only,
            cdn_bases=args.cdn_base,
            countries_path=args.countries,
            skip_cdn=args.skip_cdn,
            dry_run=args.dry_run,
            check_spa_version=not args.no_check_spa_version,
        )
    except (PrepareError, AssembleError) as exc:
        logger.error("%s", exc)
        print("error: {}".format(exc), file=sys.stderr)
        return 1
    print_result(result, port=args.port)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
