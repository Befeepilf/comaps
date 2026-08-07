#!/usr/bin/env python3
"""Unit tests for inject_spa_meta (SP-045)."""

import base64
import hashlib
import json
import os
import sys
import tempfile
import unittest


# Allow running from repo root without installing the package.
_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_TOOLS_PYTHON = os.path.dirname(os.path.dirname(_THIS_DIR))
if _TOOLS_PYTHON not in sys.path:
    sys.path.insert(0, _TOOLS_PYTHON)

from post_generation.inject_spa_meta import get_spa_hash  # noqa: E402
from post_generation.inject_spa_meta import inject_spa_meta  # noqa: E402


def _sha1_b64(data):
    return str(base64.b64encode(hashlib.sha1(data).digest()), "utf-8")


class InjectSpaMetaTest(unittest.TestCase):
    def test_inject_matching_and_omit_missing(self):
        countries = {
            "id": "Countries",
            "v": 1,
            "g": [
                {"id": "LeafWithSpa", "s": 10, "sha1_base64": "mwmA"},
                {"id": "LeafWithout", "s": 20, "sha1_base64": "mwmB"},
                {
                    "id": "Group",
                    "g": [{"id": "NestedLeaf", "s": 30, "sha1_base64": "mwmC"}],
                },
            ],
        }
        payload = b"spa-bytes-for-leaf"
        with tempfile.TemporaryDirectory() as spa_dir:
            spa_path = os.path.join(spa_dir, "LeafWithSpa.spa")
            with open(spa_path, "wb") as f:
                f.write(payload)

            count = inject_spa_meta(countries, spa_dir)
            self.assertEqual(1, count)

            with_spa = countries["g"][0]
            self.assertEqual(len(payload), with_spa["spa"])
            self.assertEqual(_sha1_b64(payload), with_spa["spa_sha1_base64"])
            self.assertEqual(get_spa_hash(spa_path), with_spa["spa_sha1_base64"])

            without = countries["g"][1]
            self.assertNotIn("spa", without)
            self.assertNotIn("spa_sha1_base64", without)

            nested = countries["g"][2]["g"][0]
            self.assertNotIn("spa", nested)
            self.assertNotIn("spa_sha1_base64", nested)

    def test_idempotent_strips_stale_keys(self):
        countries = {
            "id": "Countries",
            "v": 1,
            "g": [
                {
                    "id": "GoneSpa",
                    "s": 1,
                    "sha1_base64": "m",
                    "spa": 99,
                    "spa_sha1_base64": "stale",
                }
            ],
        }
        with tempfile.TemporaryDirectory() as spa_dir:
            count = inject_spa_meta(countries, spa_dir)
            self.assertEqual(0, count)
            leaf = countries["g"][0]
            self.assertNotIn("spa", leaf)
            self.assertNotIn("spa_sha1_base64", leaf)

    def test_no_placeholders_for_empty_dir(self):
        countries = {
            "id": "Countries",
            "v": 1,
            "g": [{"id": "A", "s": 1, "sha1_base64": "x"}],
        }
        with tempfile.TemporaryDirectory() as spa_dir:
            inject_spa_meta(countries, spa_dir)
            self.assertEqual({"id": "A", "s": 1, "sha1_base64": "x"}, countries["g"][0])


if __name__ == "__main__":
    unittest.main()
