"""Assemble a CDN-identical `.spa` publish tree (SP-050 / D8).

Builds::

    {out}/meta/maps.json
    {out}/maps/{map_series}/{publish_version}/countries.txt
    {out}/maps/{map_series}/{publish_version}/countries.txt.sig  # if --secret-key
    {out}/maps/{map_series}/{publish_version}/{leaf}.spa
    {out}/maps/{map_series}/{publish_version}/{leaf}.mwm        # if include-mwm
    {out}/inventory.json   # operator debug; SP-051 may ignore / health may read

Reuses inject_spa_meta + file_sha1_base64; optional sign_file when --secret-key
is set. Prefer hardlink then copy for binary payloads.
"""

from __future__ import annotations

import argparse
import copy
import json
import logging
import os
import shutil
import struct
import sys
import tempfile

from post_generation.inject_spa_meta import file_sha1_base64
from post_generation.inject_spa_meta import inject_spa_meta


logger = logging.getLogger(__name__)

# Little-endian fourcc "SPA1" (kSpaMagic). Raw SpaHeader at offset 0 is uncommon
# for production FilesContainer `.spa` files; used only for optional warn-only peek.
_SPA1_MAGIC = 0x31415053


class AssembleError(Exception):
    """Fail-closed assemble / verify error with a single actionable message."""


def _get_leaf_nodes(root):
    def walk(node, leaves):
        if "g" in node:
            for child in node["g"]:
                walk(child, leaves)
        else:
            leaves.append(node)

    leaves = []
    walk(root, leaves)
    return leaves


def _parse_leaves_arg(leaves_arg):
    if not leaves_arg:
        return None
    if isinstance(leaves_arg, (list, tuple, set)):
        return {str(x).strip() for x in leaves_arg if str(x).strip()}
    parts = []
    for chunk in str(leaves_arg).split(","):
        name = chunk.strip()
        if name:
            parts.append(name)
    return set(parts) if parts else None


def list_spa_leaf_ids(spa_dir):
    """Return sorted leaf ids that have `{id}.spa` under spa_dir."""
    if not os.path.isdir(spa_dir):
        raise AssembleError("spa directory not found: {}".format(spa_dir))
    ids = []
    for name in os.listdir(spa_dir):
        if name.endswith(".spa") and os.path.isfile(os.path.join(spa_dir, name)):
            ids.append(name[: -len(".spa")])
    return sorted(ids)


def try_read_spa_map_data_version(spa_path):
    """Best-effort SpaHeader map_data_version when file starts with SPA1.

    Production `.spa` files are FilesContainer-wrapped; those return None.
    """
    try:
        with open(spa_path, "rb") as f:
            blob = f.read(16)
    except OSError:
        return None
    if len(blob) < 16:
        return None
    magic, _fmt, map_ver = struct.unpack("<IIq", blob)
    if magic != _SPA1_MAGIC:
        return None
    return map_ver


def hardlink_or_copy(src, dst):
    """Prefer hardlink; fall back to copy2. Replaces existing dst."""
    parent = os.path.dirname(dst)
    if parent:
        os.makedirs(parent, exist_ok=True)
    if os.path.lexists(dst):
        os.remove(dst)
    try:
        os.link(src, dst)
        logger.debug("Hardlinked %s -> %s", src, dst)
    except OSError:
        shutil.copy2(src, dst)
        logger.debug("Copied %s -> %s", src, dst)


def _assert_source_versions(countries, map_series, data_version):
    src_v = countries.get("v")
    src_series = countries.get("map_series")
    try:
        src_v_int = int(src_v)
    except (TypeError, ValueError):
        raise AssembleError(
            "countries source \"v\" is missing or not an int: {!r}".format(src_v)
        )
    if src_v_int != int(data_version):
        raise AssembleError(
            "countries \"v\" {} does not match --data-version {}".format(
                src_v_int, data_version
            )
        )
    if src_series != map_series:
        raise AssembleError(
            "countries \"map_series\" {!r} does not match --map-series {!r}".format(
                src_series, map_series
            )
        )


def _filter_spa_allowlist(countries, allowlist):
    """If allowlist is set, drop spa ads for leaves not in the set."""
    if allowlist is None:
        return
    for leaf in _get_leaf_nodes(countries):
        leaf_id = leaf.get("id")
        if leaf_id not in allowlist:
            leaf.pop("spa", None)
            leaf.pop("spa_sha1_base64", None)


def _verify_spa_against_dir(countries, spa_dir, data_version):
    """Fail closed if injected spa size/hash disagree with on-disk files."""
    advertised = []
    for leaf in _get_leaf_nodes(countries):
        leaf_id = leaf.get("id")
        if not leaf_id:
            continue
        has_spa = "spa" in leaf
        has_hash = "spa_sha1_base64" in leaf
        if has_spa != has_hash:
            raise AssembleError(
                "leaf {!r} has inconsistent spa keys (both or neither required)".format(
                    leaf_id
                )
            )
        if not has_spa:
            continue
        spa_path = os.path.join(spa_dir, "{}.spa".format(leaf_id))
        if not os.path.isfile(spa_path):
            raise AssembleError(
                "advertised spa for {!r} but file missing: {}".format(leaf_id, spa_path)
            )
        size = os.path.getsize(spa_path)
        digest = file_sha1_base64(spa_path)
        if size != leaf["spa"]:
            raise AssembleError(
                "spa size mismatch for {!r}: file {} != countries {}".format(
                    leaf_id, size, leaf["spa"]
                )
            )
        if digest != leaf["spa_sha1_base64"]:
            raise AssembleError(
                "spa sha1 mismatch for {!r}: file does not match countries spa_sha1_base64".format(
                    leaf_id
                )
            )
        hdr_ver = try_read_spa_map_data_version(spa_path)
        if hdr_ver is not None and int(hdr_ver) != int(data_version):
            logger.warning(
                "spa header map_data_version %s for %s differs from --data-version %s",
                hdr_ver,
                leaf_id,
                data_version,
            )
        advertised.append(leaf_id)
        logger.info("Verified spa %s (%s bytes)", leaf_id, size)
    return advertised


def _verify_mwm_against_dir(countries, mwm_dir, leaf_ids):
    """Fail closed if MWM size/hash disagree with countries s / sha1_base64."""
    if not mwm_dir or not os.path.isdir(mwm_dir):
        raise AssembleError("mwm directory not found: {}".format(mwm_dir))
    for leaf_id in leaf_ids:
        leaf = None
        for candidate in _get_leaf_nodes(countries):
            if candidate.get("id") == leaf_id:
                leaf = candidate
                break
        if leaf is None:
            raise AssembleError("advertised leaf {!r} missing from countries".format(leaf_id))
        mwm_path = os.path.join(mwm_dir, "{}.mwm".format(leaf_id))
        if not os.path.isfile(mwm_path):
            raise AssembleError(
                "missing mwm for advertised leaf {!r}: {}".format(leaf_id, mwm_path)
            )
        if "s" not in leaf or "sha1_base64" not in leaf:
            raise AssembleError(
                "leaf {!r} missing countries \"s\" / \"sha1_base64\" for mwm verify".format(
                    leaf_id
                )
            )
        size = os.path.getsize(mwm_path)
        digest = file_sha1_base64(mwm_path)
        if size != leaf["s"]:
            raise AssembleError(
                "mwm size mismatch for {!r}: file {} != countries \"s\" {}".format(
                    leaf_id, size, leaf["s"]
                )
            )
        if digest != leaf["sha1_base64"]:
            raise AssembleError(
                "mwm sha1 mismatch for {!r}: file does not match countries sha1_base64".format(
                    leaf_id
                )
            )
        logger.info("Verified mwm %s (%s bytes)", leaf_id, size)


def build_maps_json(map_series, publish_version):
    return {
        "map-series": {
            str(map_series): {
                "latest": int(publish_version),
                "status": "active",
            }
        }
    }


def build_inventory(
    map_series,
    data_version,
    publish_version,
    countries,
    spa_dir,
    mwm_dir,
    include_mwm,
    advertised_ids,
):
    leaves_inv = []
    advertised_set = set(advertised_ids)
    for leaf in _get_leaf_nodes(countries):
        leaf_id = leaf.get("id")
        if not leaf_id:
            continue
        if leaf_id not in advertised_set and "spa" not in leaf:
            # Skip unadvertised leaves unless they somehow have spa keys.
            continue
        entry = {
            "id": leaf_id,
            "advertised": leaf_id in advertised_set,
            "spa_bytes": leaf.get("spa"),
            "mwm_bytes": None,
            "publish_version": int(publish_version),
        }
        if include_mwm and mwm_dir:
            mwm_path = os.path.join(mwm_dir, "{}.mwm".format(leaf_id))
            if os.path.isfile(mwm_path):
                entry["mwm_bytes"] = os.path.getsize(mwm_path)
            elif "s" in leaf:
                entry["mwm_bytes"] = leaf["s"]
        leaves_inv.append(entry)
    return {
        "map_series": map_series,
        "data_version": int(data_version),
        "publish_version": int(publish_version),
        "include_mwm": bool(include_mwm),
        "leaves": leaves_inv,
    }


def version_dir(out, map_series, publish_version):
    return os.path.join(out, "maps", str(map_series), str(publish_version))


def write_publish_tree(
    out,
    countries,
    map_series,
    publish_version,
    spa_dir,
    mwm_dir,
    include_mwm,
    advertised_ids,
    secret_key,
    inventory,
):
    """Write the tree under out. Pre-validated inputs assumed."""
    os.makedirs(out, exist_ok=True)
    vdir = version_dir(out, map_series, publish_version)
    # Fresh version dir contents for this publish version.
    if os.path.isdir(vdir):
        shutil.rmtree(vdir)
    os.makedirs(vdir, exist_ok=True)

    meta_dir = os.path.join(out, "meta")
    os.makedirs(meta_dir, exist_ok=True)
    maps_json_path = os.path.join(meta_dir, "maps.json")
    with open(maps_json_path, "w") as f:
        json.dump(build_maps_json(map_series, publish_version), f, indent=2)
        f.write("\n")
    logger.info("Wrote %s", maps_json_path)

    countries_path = os.path.join(vdir, "countries.txt")
    with open(countries_path, "w") as f:
        json.dump(countries, f, ensure_ascii=False, indent=1)
        f.write("\n")
    logger.info("Wrote %s", countries_path)

    if secret_key:
        from maps_generator.utils.file import sign_file

        if not os.path.isfile(secret_key):
            raise AssembleError("secret key not found: {}".format(secret_key))
        sig_path = sign_file(countries_path, secret_key)
        logger.info("Signed countries -> %s", sig_path)

    for leaf_id in advertised_ids:
        src = os.path.join(spa_dir, "{}.spa".format(leaf_id))
        dst = os.path.join(vdir, "{}.spa".format(leaf_id))
        hardlink_or_copy(src, dst)
        logger.info("Placed spa %s", dst)

    if include_mwm:
        for leaf_id in advertised_ids:
            src = os.path.join(mwm_dir, "{}.mwm".format(leaf_id))
            dst = os.path.join(vdir, "{}.mwm".format(leaf_id))
            hardlink_or_copy(src, dst)
            logger.info("Placed mwm %s", dst)

    inv_path = os.path.join(out, "inventory.json")
    with open(inv_path, "w") as f:
        json.dump(inventory, f, indent=2)
        f.write("\n")
    logger.info("Wrote %s", inv_path)


def verify_existing_tree(
    out,
    map_series,
    data_version,
    publish_version,
    include_mwm,
    leaves_allowlist=None,
):
    """Re-check an existing --out without writing."""
    maps_json_path = os.path.join(out, "meta", "maps.json")
    if not os.path.isfile(maps_json_path):
        raise AssembleError("missing {}".format(maps_json_path))
    with open(maps_json_path) as f:
        maps_json = json.load(f)
    series_block = (maps_json.get("map-series") or {}).get(str(map_series))
    if not series_block:
        raise AssembleError(
            "maps.json missing map-series entry for {!r}".format(map_series)
        )
    if int(series_block.get("latest")) != int(publish_version):
        raise AssembleError(
            "maps.json latest {} != publish version {}".format(
                series_block.get("latest"), publish_version
            )
        )
    if series_block.get("status") != "active":
        raise AssembleError(
            "maps.json status {!r} != 'active'".format(series_block.get("status"))
        )

    vdir = version_dir(out, map_series, publish_version)
    countries_path = os.path.join(vdir, "countries.txt")
    if not os.path.isfile(countries_path):
        raise AssembleError("missing {}".format(countries_path))
    with open(countries_path) as f:
        countries = json.load(f)

    if int(countries.get("v")) != int(publish_version):
        raise AssembleError(
            "output countries \"v\" {} != publish version {}".format(
                countries.get("v"), publish_version
            )
        )
    if countries.get("map_series") != map_series:
        raise AssembleError(
            "output countries map_series {!r} != {!r}".format(
                countries.get("map_series"), map_series
            )
        )

    advertised = []
    for leaf in _get_leaf_nodes(countries):
        leaf_id = leaf.get("id")
        if not leaf_id or "spa" not in leaf:
            continue
        if leaves_allowlist is not None and leaf_id not in leaves_allowlist:
            continue
        spa_path = os.path.join(vdir, "{}.spa".format(leaf_id))
        if not os.path.isfile(spa_path):
            raise AssembleError("missing spa in tree: {}".format(spa_path))
        size = os.path.getsize(spa_path)
        digest = file_sha1_base64(spa_path)
        if size != leaf["spa"] or digest != leaf["spa_sha1_base64"]:
            raise AssembleError("spa verify failed for {!r} under {}".format(leaf_id, vdir))
        advertised.append(leaf_id)
        logger.info("verify-only: spa ok %s", leaf_id)

        if include_mwm:
            mwm_path = os.path.join(vdir, "{}.mwm".format(leaf_id))
            if not os.path.isfile(mwm_path):
                raise AssembleError("missing mwm in tree: {}".format(mwm_path))
            msize = os.path.getsize(mwm_path)
            mdigest = file_sha1_base64(mwm_path)
            if msize != leaf.get("s") or mdigest != leaf.get("sha1_base64"):
                raise AssembleError(
                    "mwm verify failed for {!r} under {}".format(leaf_id, vdir)
                )
            logger.info("verify-only: mwm ok %s", leaf_id)

    logger.info("verify-only: ok (%s advertised leaves)", len(advertised))
    return 0


def assemble_spa_publish_tree(
    countries_path,
    spa_dir,
    out,
    map_series,
    data_version,
    mwm_dir=None,
    publish_version=None,
    leaves=None,
    secret_key=None,
    include_mwm=True,
    spa_only=False,
    dry_run=False,
    verify_only=False,
):
    """Core assemble entry. Returns 0 on success; raises AssembleError on failure."""
    if spa_only:
        include_mwm = False
    if publish_version is None:
        publish_version = data_version
    allowlist = _parse_leaves_arg(leaves)

    if verify_only:
        return verify_existing_tree(
            out=out,
            map_series=map_series,
            data_version=data_version,
            publish_version=publish_version,
            include_mwm=include_mwm,
            leaves_allowlist=allowlist,
        )

    if include_mwm and not mwm_dir:
        raise AssembleError("--mwm-dir is required unless --spa-only / --no-include-mwm")

    with open(countries_path) as f:
        countries = json.load(f)

    _assert_source_versions(countries, map_series, data_version)
    countries = copy.deepcopy(countries)

    # Inject from spa-dir; optionally restrict to --leaves allowlist.
    inject_spa_meta(countries, spa_dir)
    _filter_spa_allowlist(countries, allowlist)

    if int(publish_version) != int(data_version):
        countries["v"] = int(publish_version)
        logger.info(
            "Channel A publish-version bump: countries \"v\" %s -> %s",
            data_version,
            publish_version,
        )

    advertised_ids = _verify_spa_against_dir(countries, spa_dir, data_version)
    if allowlist is not None:
        # Only assemble allowlisted advertised leaves (intersection).
        advertised_ids = [i for i in advertised_ids if i in allowlist]

    if include_mwm:
        _verify_mwm_against_dir(countries, mwm_dir, advertised_ids)

    inventory = build_inventory(
        map_series=map_series,
        data_version=data_version,
        publish_version=publish_version,
        countries=countries,
        spa_dir=spa_dir,
        mwm_dir=mwm_dir,
        include_mwm=include_mwm,
        advertised_ids=advertised_ids,
    )

    vdir = version_dir(out, map_series, publish_version)
    plan_lines = [
        "map_series={}".format(map_series),
        "data_version={}".format(data_version),
        "publish_version={}".format(publish_version),
        "include_mwm={}".format(include_mwm),
        "version_dir={}".format(vdir),
        "advertised_leaves={}".format(len(advertised_ids)),
        "meta/maps.json latest={}".format(publish_version),
        "inventory.json leaves={}".format(len(inventory["leaves"])),
    ]
    for leaf_id in advertised_ids:
        plan_lines.append("  spa: {}.spa".format(leaf_id))
        if include_mwm:
            plan_lines.append("  mwm: {}.mwm".format(leaf_id))
    if secret_key:
        plan_lines.append("  sign: countries.txt.sig with {}".format(secret_key))

    if dry_run:
        print("dry-run plan:")
        for line in plan_lines:
            print("  " + line)
        return 0

    # Write via staging under out's parent so a mid-write failure does not leave
    # a half-updated version dir as the only copy.
    out_abs = os.path.abspath(out)
    parent = os.path.dirname(out_abs) or "."
    os.makedirs(parent, exist_ok=True)
    staging = tempfile.mkdtemp(prefix=".spa_publish_staging_", dir=parent)
    try:
        write_publish_tree(
            out=staging,
            countries=countries,
            map_series=map_series,
            publish_version=publish_version,
            spa_dir=spa_dir,
            mwm_dir=mwm_dir,
            include_mwm=include_mwm,
            advertised_ids=advertised_ids,
            secret_key=secret_key,
            inventory=inventory,
        )
        # Merge staging into out: replace maps/{series}/{ver}, meta/maps.json, inventory.
        os.makedirs(out_abs, exist_ok=True)
        final_vdir = version_dir(out_abs, map_series, publish_version)
        staging_vdir = version_dir(staging, map_series, publish_version)
        if os.path.isdir(final_vdir):
            shutil.rmtree(final_vdir)
        os.makedirs(os.path.dirname(final_vdir), exist_ok=True)
        shutil.move(staging_vdir, final_vdir)

        meta_dst = os.path.join(out_abs, "meta")
        os.makedirs(meta_dst, exist_ok=True)
        shutil.copy2(
            os.path.join(staging, "meta", "maps.json"),
            os.path.join(meta_dst, "maps.json"),
        )
        shutil.copy2(
            os.path.join(staging, "inventory.json"),
            os.path.join(out_abs, "inventory.json"),
        )
    except Exception:
        # Staging discarded below; if final_vdir was removed already, fail closed
        # without a corrupt half-tree from this run's staging merge.
        raise
    finally:
        shutil.rmtree(staging, ignore_errors=True)

    logger.info("Assembled publish tree at %s", out_abs)
    return 0


def build_arg_parser():
    parser = argparse.ArgumentParser(
        description="Assemble CDN-identical .spa publish tree (SP-050)."
    )
    parser.add_argument("--countries", required=True, help="Source countries.txt")
    parser.add_argument("--spa-dir", required=True, help="Directory of {leaf}.spa")
    parser.add_argument(
        "--mwm-dir",
        default=None,
        help="Directory of {leaf}.mwm (required unless --spa-only)",
    )
    parser.add_argument("--out", required=True, help="Output publish root")
    parser.add_argument("--map-series", required=True, help="Must match MAP_SERIES")
    parser.add_argument(
        "--data-version",
        type=int,
        required=True,
        help="Source countries \"v\" / MWM data version",
    )
    parser.add_argument(
        "--publish-version",
        type=int,
        default=None,
        help="Optional Channel A meta-only bump (output dir + countries v + maps.json)",
    )
    parser.add_argument(
        "--leaves",
        default=None,
        help="Optional comma-separated leaf allowlist (default: all .spa in spa-dir)",
    )
    parser.add_argument(
        "--secret-key",
        default=None,
        help="Optional Ed25519 PEM path; writes countries.txt.sig",
    )
    parser.add_argument(
        "--include-mwm",
        action=argparse.BooleanOptionalAction,
        default=None,
        help="Copy/link .mwm into version dir (default: true unless --spa-only)",
    )
    parser.add_argument(
        "--spa-only",
        action="store_true",
        help="Only place .spa + patched countries (no MWM required)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print planned actions; write nothing",
    )
    parser.add_argument(
        "--verify-only",
        action="store_true",
        help="Re-check existing --out without writing",
    )
    parser.add_argument("--verbose", action="store_true", help="Verbose logging")
    return parser


def main(argv=None):
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(levelname)s %(name)s: %(message)s",
    )

    include_mwm = args.include_mwm
    if include_mwm is None:
        include_mwm = not args.spa_only
    if args.spa_only:
        include_mwm = False

    try:
        return assemble_spa_publish_tree(
            countries_path=args.countries,
            spa_dir=args.spa_dir,
            mwm_dir=args.mwm_dir,
            out=args.out,
            map_series=args.map_series,
            data_version=args.data_version,
            publish_version=args.publish_version,
            leaves=args.leaves,
            secret_key=args.secret_key,
            include_mwm=include_mwm,
            spa_only=args.spa_only,
            dry_run=args.dry_run,
            verify_only=args.verify_only,
        )
    except AssembleError as exc:
        logger.error("%s", exc)
        print("error: {}".format(exc), file=sys.stderr)
        return 1
    except FileNotFoundError as exc:
        logger.error("%s", exc)
        print("error: {}".format(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
