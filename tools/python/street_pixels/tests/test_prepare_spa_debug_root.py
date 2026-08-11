#!/usr/bin/env python3
"""Unit tests for prepare_spa_debug_root."""

import json
import os
import sys
import tempfile
import unittest
from unittest import mock


_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_TOOLS_PYTHON = os.path.dirname(os.path.dirname(_THIS_DIR))
if _TOOLS_PYTHON not in sys.path:
    sys.path.insert(0, _TOOLS_PYTHON)

from street_pixels.prepare_spa_debug_root import PrepareError  # noqa: E402
from street_pixels.prepare_spa_debug_root import countries_relative_url  # noqa: E402
from street_pixels.prepare_spa_debug_root import fetch_latest_data_version  # noqa: E402
from street_pixels.prepare_spa_debug_root import prepare_spa_debug_root  # noqa: E402


SERIES = "2026.06.28"
DATA_V = 260803
LEAF = "Finland_Southern Finland_Helsinki"


def _countries_json(leaf=LEAF, v=DATA_V, series=SERIES, mwm_size=100):
    return {
        "id": "Countries",
        "v": v,
        "map_series": series,
        "g": [{"id": leaf, "s": mwm_size, "sha1_base64": "AAAAAAAAAAAAAAAAAAAAAAAAAAA="}],
    }


def _maps_json(latest=DATA_V, series=SERIES):
    return {
        "map-series": {
            series: {"latest": latest, "status": "active"},
        }
    }


class FetchLatestTest(unittest.TestCase):
    def test_parses_latest(self):
        body = json.dumps(_maps_json()).encode("utf-8")

        def fake_get(url, timeout_s=60):
            self.assertIn("meta/maps.json", url)
            return body

        with mock.patch(
            "street_pixels.prepare_spa_debug_root._http_get", side_effect=fake_get
        ):
            latest, base, meta = fetch_latest_data_version(
                SERIES, cdn_bases=["https://example.test/"]
            )
        self.assertEqual(DATA_V, latest)
        self.assertEqual("https://example.test/", base)
        self.assertEqual("active", meta["map-series"][SERIES]["status"])


class PrepareSpaDebugRootTest(unittest.TestCase):
    def _spa_dir(self, tmp, payload=b"fake-spa-bytes"):
        spa_dir = os.path.join(tmp, "spa")
        os.makedirs(spa_dir)
        with open(os.path.join(spa_dir, "{}.spa".format(LEAF)), "wb") as f:
            f.write(payload)
        return spa_dir

    def test_local_countries_spa_only_layout(self):
        with tempfile.TemporaryDirectory() as tmp:
            spa_dir = self._spa_dir(tmp)
            countries_path = os.path.join(tmp, "countries.txt")
            with open(countries_path, "w") as f:
                json.dump(_countries_json(), f)
            out = os.path.join(tmp, "out")
            result = prepare_spa_debug_root(
                spa_dir=spa_dir,
                out=out,
                map_series=SERIES,
                countries_path=countries_path,
                skip_cdn=True,
                channel="serve-only",
                spa_only=True,
            )
            self.assertEqual(DATA_V, result["data_version"])
            vdir = os.path.join(out, "maps", SERIES, str(DATA_V))
            self.assertTrue(os.path.isfile(os.path.join(vdir, "{}.spa".format(LEAF))))
            self.assertTrue(os.path.isfile(os.path.join(vdir, "countries.txt")))
            with open(os.path.join(vdir, "countries.txt")) as f:
                countries = json.load(f)
            self.assertIn("spa", countries["g"][0])
            self.assertFalse(os.path.isfile(os.path.join(vdir, "{}.mwm".format(LEAF))))
            with open(os.path.join(out, "meta", "maps.json")) as f:
                maps_json = json.load(f)
            self.assertEqual(DATA_V, maps_json["map-series"][SERIES]["latest"])

    def test_cdn_path_downloads_then_assembles(self):
        with tempfile.TemporaryDirectory() as tmp:
            spa_dir = self._spa_dir(tmp)
            out = os.path.join(tmp, "out")
            meta_body = json.dumps(_maps_json()).encode("utf-8")
            countries_body = json.dumps(_countries_json()).encode("utf-8")

            def fake_get(url, timeout_s=60):
                if url.endswith("meta/maps.json"):
                    return meta_body
                if url.endswith(countries_relative_url(SERIES, DATA_V)):
                    return countries_body
                raise PrepareError("unexpected url {}".format(url))

            with mock.patch(
                "street_pixels.prepare_spa_debug_root._http_get", side_effect=fake_get
            ):
                result = prepare_spa_debug_root(
                    spa_dir=spa_dir,
                    out=out,
                    map_series=SERIES,
                    channel="serve-only",
                    spa_only=True,
                    cdn_bases=["https://mirror.test/"],
                )
            self.assertEqual(DATA_V, result["data_version"])
            self.assertTrue(
                os.path.isfile(
                    os.path.join(out, "maps", SERIES, str(DATA_V), "{}.spa".format(LEAF))
                )
            )
            self.assertIn("Finland_Southern%20Finland_Helsinki.spa", result["sample_url_path"])

    def test_channel_b_writes_inject_without_touching_repo(self):
        with tempfile.TemporaryDirectory() as tmp:
            spa_dir = self._spa_dir(tmp)
            countries_path = os.path.join(tmp, "countries.txt")
            with open(countries_path, "w") as f:
                json.dump(_countries_json(), f)
            out = os.path.join(tmp, "out")
            # Sentinel: a fake "repo" countries that must stay untouched.
            repo_countries = os.path.join(tmp, "repo_data_countries.txt")
            with open(repo_countries, "w") as f:
                f.write("SENTINEL\n")
            with open(repo_countries) as f:
                before = f.read()
            result = prepare_spa_debug_root(
                spa_dir=spa_dir,
                out=out,
                countries_path=countries_path,
                skip_cdn=True,
                channel="b",
                spa_only=True,
            )
            self.assertTrue(os.path.isfile(result["channel_b_countries"]))
            with open(result["channel_b_countries"]) as f:
                patched = json.load(f)
            self.assertIn("spa", patched["g"][0])
            with open(repo_countries) as f:
                self.assertEqual(before, f.read())

    def test_version_mismatch_local_countries_uses_file_v(self):
        with tempfile.TemporaryDirectory() as tmp:
            spa_dir = self._spa_dir(tmp)
            countries_path = os.path.join(tmp, "countries.txt")
            with open(countries_path, "w") as f:
                json.dump(_countries_json(v=260803), f)
            out = os.path.join(tmp, "out")
            result = prepare_spa_debug_root(
                spa_dir=spa_dir,
                out=out,
                countries_path=countries_path,
                skip_cdn=True,
                data_version=260714,  # ignored when --countries provided
                spa_only=True,
            )
            self.assertEqual(260803, result["data_version"])

    def test_skip_cdn_without_countries_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            spa_dir = self._spa_dir(tmp)
            with self.assertRaises(PrepareError):
                prepare_spa_debug_root(
                    spa_dir=spa_dir,
                    out=os.path.join(tmp, "out"),
                    skip_cdn=True,
                    spa_only=True,
                )


if __name__ == "__main__":
    unittest.main()
