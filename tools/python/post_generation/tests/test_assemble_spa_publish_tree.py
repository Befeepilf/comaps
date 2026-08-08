#!/usr/bin/env python3
"""Unit tests for assemble_spa_publish_tree (SP-050)."""

import base64
import hashlib
import json
import os
import sys
import tempfile
import unittest


_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_TOOLS_PYTHON = os.path.dirname(os.path.dirname(_THIS_DIR))
if _TOOLS_PYTHON not in sys.path:
    sys.path.insert(0, _TOOLS_PYTHON)

from post_generation.assemble_spa_publish_tree import AssembleError  # noqa: E402
from post_generation.assemble_spa_publish_tree import assemble_spa_publish_tree  # noqa: E402
from post_generation.inject_spa_meta import file_sha1_base64  # noqa: E402


def _sha1_b64(data):
    return str(base64.b64encode(hashlib.sha1(data).digest()), "utf-8")


def _write(path, data):
    parent = os.path.dirname(path)
    if parent:
        os.makedirs(parent, exist_ok=True)
    mode = "wb" if isinstance(data, (bytes, bytearray)) else "w"
    with open(path, mode) as f:
        f.write(data)


class AssembleSpaPublishTreeTest(unittest.TestCase):
    SERIES = "2026.06.28"
    DATA_V = 260714

    def _countries(self, leaf_id, mwm_bytes, mwm_hash, series=None, v=None):
        return {
            "id": "Countries",
            "v": self.DATA_V if v is None else v,
            "map_series": self.SERIES if series is None else series,
            "g": [
                {
                    "id": leaf_id,
                    "s": len(mwm_bytes),
                    "sha1_base64": mwm_hash,
                }
            ],
        }

    def _fixture(self, tmp, spa_payload=b"fake-spa-bytes", mwm_payload=b"fake-mwm-bytes"):
        leaf = "Finland_Helsinki"
        spa_dir = os.path.join(tmp, "spa")
        mwm_dir = os.path.join(tmp, "mwm")
        out = os.path.join(tmp, "out")
        _write(os.path.join(spa_dir, "{}.spa".format(leaf)), spa_payload)
        _write(os.path.join(mwm_dir, "{}.mwm".format(leaf)), mwm_payload)
        countries_path = os.path.join(tmp, "countries.txt")
        countries = self._countries(leaf, mwm_payload, _sha1_b64(mwm_payload))
        _write(countries_path, json.dumps(countries, indent=1) + "\n")
        return leaf, spa_dir, mwm_dir, out, countries_path

    def test_happy_path_layout_and_hashes(self):
        with tempfile.TemporaryDirectory() as tmp:
            leaf, spa_dir, mwm_dir, out, countries_path = self._fixture(tmp)
            rc = assemble_spa_publish_tree(
                countries_path=countries_path,
                spa_dir=spa_dir,
                mwm_dir=mwm_dir,
                out=out,
                map_series=self.SERIES,
                data_version=self.DATA_V,
            )
            self.assertEqual(0, rc)

            vdir = os.path.join(out, "maps", self.SERIES, str(self.DATA_V))
            self.assertTrue(os.path.isfile(os.path.join(vdir, "countries.txt")))
            self.assertTrue(os.path.isfile(os.path.join(vdir, "{}.spa".format(leaf))))
            self.assertTrue(os.path.isfile(os.path.join(vdir, "{}.mwm".format(leaf))))

            with open(os.path.join(out, "meta", "maps.json")) as f:
                maps_json = json.load(f)
            self.assertEqual(
                {
                    "map-series": {
                        self.SERIES: {"latest": self.DATA_V, "status": "active"}
                    }
                },
                maps_json,
            )

            with open(os.path.join(vdir, "countries.txt")) as f:
                countries = json.load(f)
            leaf_node = countries["g"][0]
            spa_path = os.path.join(vdir, "{}.spa".format(leaf))
            self.assertEqual(os.path.getsize(spa_path), leaf_node["spa"])
            self.assertEqual(file_sha1_base64(spa_path), leaf_node["spa_sha1_base64"])
            mwm_path = os.path.join(vdir, "{}.mwm".format(leaf))
            self.assertEqual(os.path.getsize(mwm_path), leaf_node["s"])
            self.assertEqual(file_sha1_base64(mwm_path), leaf_node["sha1_base64"])

            self.assertTrue(os.path.isfile(os.path.join(out, "inventory.json")))
            with open(os.path.join(out, "inventory.json")) as f:
                inv = json.load(f)
            self.assertEqual(self.DATA_V, inv["publish_version"])
            self.assertEqual(1, len(inv["leaves"]))

            # verify-only passes on the tree we just wrote
            self.assertEqual(
                0,
                assemble_spa_publish_tree(
                    countries_path=countries_path,
                    spa_dir=spa_dir,
                    mwm_dir=mwm_dir,
                    out=out,
                    map_series=self.SERIES,
                    data_version=self.DATA_V,
                    verify_only=True,
                ),
            )

    def test_mwm_hash_mismatch_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            leaf, spa_dir, mwm_dir, out, countries_path = self._fixture(tmp)
            # Corrupt countries mwm hash so on-disk mwm disagrees.
            with open(countries_path) as f:
                countries = json.load(f)
            countries["g"][0]["sha1_base64"] = _sha1_b64(b"not-the-mwm")
            _write(countries_path, json.dumps(countries, indent=1) + "\n")

            with self.assertRaises(AssembleError) as ctx:
                assemble_spa_publish_tree(
                    countries_path=countries_path,
                    spa_dir=spa_dir,
                    mwm_dir=mwm_dir,
                    out=out,
                    map_series=self.SERIES,
                    data_version=self.DATA_V,
                )
            self.assertIn("mwm sha1 mismatch", str(ctx.exception))
            # No version dir / maps.json from a failed assemble.
            self.assertFalse(
                os.path.isdir(os.path.join(out, "maps", self.SERIES, str(self.DATA_V)))
            )
            self.assertFalse(os.path.isfile(os.path.join(out, "meta", "maps.json")))

    def test_publish_version_bump(self):
        publish_v = 260715
        with tempfile.TemporaryDirectory() as tmp:
            leaf, spa_dir, mwm_dir, out, countries_path = self._fixture(tmp)
            mwm_hash_before = None
            with open(countries_path) as f:
                mwm_hash_before = json.load(f)["g"][0]["sha1_base64"]

            rc = assemble_spa_publish_tree(
                countries_path=countries_path,
                spa_dir=spa_dir,
                mwm_dir=mwm_dir,
                out=out,
                map_series=self.SERIES,
                data_version=self.DATA_V,
                publish_version=publish_v,
            )
            self.assertEqual(0, rc)

            vdir = os.path.join(out, "maps", self.SERIES, str(publish_v))
            self.assertTrue(os.path.isdir(vdir))
            self.assertFalse(
                os.path.isdir(os.path.join(out, "maps", self.SERIES, str(self.DATA_V)))
            )

            with open(os.path.join(vdir, "countries.txt")) as f:
                countries = json.load(f)
            self.assertEqual(publish_v, countries["v"])
            self.assertEqual(mwm_hash_before, countries["g"][0]["sha1_base64"])

            with open(os.path.join(out, "meta", "maps.json")) as f:
                maps_json = json.load(f)
            self.assertEqual(publish_v, maps_json["map-series"][self.SERIES]["latest"])
            self.assertTrue(os.path.isfile(os.path.join(vdir, "{}.mwm".format(leaf))))

    def test_spa_only_skips_mwm(self):
        with tempfile.TemporaryDirectory() as tmp:
            leaf, spa_dir, _mwm_dir, out, countries_path = self._fixture(tmp)
            rc = assemble_spa_publish_tree(
                countries_path=countries_path,
                spa_dir=spa_dir,
                mwm_dir=None,
                out=out,
                map_series=self.SERIES,
                data_version=self.DATA_V,
                spa_only=True,
            )
            self.assertEqual(0, rc)
            vdir = os.path.join(out, "maps", self.SERIES, str(self.DATA_V))
            self.assertTrue(os.path.isfile(os.path.join(vdir, "{}.spa".format(leaf))))
            self.assertFalse(os.path.isfile(os.path.join(vdir, "{}.mwm".format(leaf))))
            with open(os.path.join(vdir, "countries.txt")) as f:
                countries = json.load(f)
            self.assertIn("spa", countries["g"][0])

    def test_series_mismatch_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            _leaf, spa_dir, mwm_dir, out, countries_path = self._fixture(tmp)
            with self.assertRaises(AssembleError) as ctx:
                assemble_spa_publish_tree(
                    countries_path=countries_path,
                    spa_dir=spa_dir,
                    mwm_dir=mwm_dir,
                    out=out,
                    map_series="1999.01.01",
                    data_version=self.DATA_V,
                )
            self.assertIn("map_series", str(ctx.exception))

    def test_dry_run_writes_nothing(self):
        with tempfile.TemporaryDirectory() as tmp:
            _leaf, spa_dir, mwm_dir, out, countries_path = self._fixture(tmp)
            from io import StringIO
            from contextlib import redirect_stdout

            buf = StringIO()
            with redirect_stdout(buf):
                rc = assemble_spa_publish_tree(
                    countries_path=countries_path,
                    spa_dir=spa_dir,
                    mwm_dir=mwm_dir,
                    out=out,
                    map_series=self.SERIES,
                    data_version=self.DATA_V,
                    dry_run=True,
                )
            self.assertEqual(0, rc)
            self.assertIn("dry-run plan:", buf.getvalue())
            self.assertFalse(os.path.exists(out) and os.listdir(out))

    def test_empty_advertised_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            leaf, spa_dir, mwm_dir, out, countries_path = self._fixture(tmp)
            os.remove(os.path.join(spa_dir, "{}.spa".format(leaf)))
            with self.assertRaises(AssembleError) as ctx:
                assemble_spa_publish_tree(
                    countries_path=countries_path,
                    spa_dir=spa_dir,
                    mwm_dir=mwm_dir,
                    out=out,
                    map_series=self.SERIES,
                    data_version=self.DATA_V,
                )
            self.assertIn("no spa advertisements", str(ctx.exception))

    def test_atomic_replace_keeps_prior_on_second_assemble(self):
        with tempfile.TemporaryDirectory() as tmp:
            leaf, spa_dir, mwm_dir, out, countries_path = self._fixture(tmp)
            self.assertEqual(
                0,
                assemble_spa_publish_tree(
                    countries_path=countries_path,
                    spa_dir=spa_dir,
                    mwm_dir=mwm_dir,
                    out=out,
                    map_series=self.SERIES,
                    data_version=self.DATA_V,
                ),
            )
            vdir = os.path.join(out, "maps", self.SERIES, str(self.DATA_V))
            first_spa = os.path.join(vdir, "{}.spa".format(leaf))
            self.assertTrue(os.path.isfile(first_spa))
            _write(os.path.join(spa_dir, "{}.spa".format(leaf)), b"second-spa-payload")
            # countries spa hash must match new bytes after re-inject — reassemble
            self.assertEqual(
                0,
                assemble_spa_publish_tree(
                    countries_path=countries_path,
                    spa_dir=spa_dir,
                    mwm_dir=mwm_dir,
                    out=out,
                    map_series=self.SERIES,
                    data_version=self.DATA_V,
                ),
            )
            with open(os.path.join(vdir, "{}.spa".format(leaf)), "rb") as f:
                self.assertEqual(b"second-spa-payload", f.read())
            self.assertFalse(os.path.isdir(vdir + ".old"))
            self.assertFalse(os.path.isdir(vdir + ".new"))


if __name__ == "__main__":
    unittest.main()
