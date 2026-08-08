"""Inject optional spa / spa_sha1_base64 leaf fields into countries.txt.

Given a countries JSON tree and a publish directory of `{leafId}.spa` files,
sets size + SHA-1 (base64) on matching leaves. Leaves without a sidecar omit
both keys (no placeholders). Re-runs strip stale spa keys when the file is gone.

SPD-028 / SP-045.
"""

import argparse
import base64
import hashlib
import json
import logging
import os


logger = logging.getLogger(__name__)


def _get_leaf_nodes(root):
    """Collect leaf country nodes (no `"g"` children), mirroring inject_promo_ids."""

    def walk(node, leaves):
        if "g" in node:
            for child in node["g"]:
                walk(child, leaves)
        else:
            leaves.append(node)

    leaves = []
    walk(root, leaves)
    return leaves


def file_sha1_base64(path):
    """SHA-1 digest of a file, base64-encoded (same style as get_mwm_hash / spa)."""
    h = hashlib.sha1()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            h.update(chunk)
    return str(base64.b64encode(h.digest()), "utf-8")


def get_spa_hash(spa_path):
    """SHA-1 digest of the `.spa` file, base64-encoded (alias of file_sha1_base64)."""
    return file_sha1_base64(spa_path)


def inject_spa_meta(countries, spa_dir):
    """Patch leaf nodes in-place from `{spa_dir}/{id}.spa`.

    Returns the number of leaves that advertise spa meta after the pass.
    """
    if not os.path.isdir(spa_dir):
        raise FileNotFoundError("spa directory not found: {}".format(spa_dir))

    advertised = 0
    for leaf in _get_leaf_nodes(countries):
        leaf_id = leaf.get("id")
        if not leaf_id:
            continue
        spa_path = os.path.join(spa_dir, "{}.spa".format(leaf_id))
        if os.path.isfile(spa_path):
            size = os.path.getsize(spa_path)
            leaf["spa"] = size
            leaf["spa_sha1_base64"] = file_sha1_base64(spa_path)
            advertised += 1
            logger.info("Advertised spa for %s (%s bytes)", leaf_id, size)
        else:
            # Idempotent re-run: drop stale keys when the sidecar is missing.
            leaf.pop("spa", None)
            leaf.pop("spa_sha1_base64", None)

    return advertised


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Inject spa / spa_sha1_base64 into countries.txt from a .spa publish dir."
    )
    parser.add_argument("--countries", required=True, help="Input countries.json / countries.txt")
    parser.add_argument(
        "--spa-dir",
        required=True,
        help="Directory of {mwmLeafId}.spa files (SP-044 publish tree)",
    )
    parser.add_argument("--output", required=True, help="Output countries.txt path")
    args = parser.parse_args(argv)

    with open(args.countries) as f:
        countries = json.load(f)

    count = inject_spa_meta(countries, args.spa_dir)
    logger.info("Advertised spa on %s leaves", count)

    with open(args.output, "w") as f:
        json.dump(countries, f, ensure_ascii=False, indent=1)
        f.write("\n")

    return 0


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    raise SystemExit(main())
