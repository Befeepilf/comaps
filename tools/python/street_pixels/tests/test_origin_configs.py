#!/usr/bin/env python3
"""Unit tests for committed VPS origin snippets (SP-102)."""

import os
import unittest


_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_MODULE_DIR = os.path.dirname(_THIS_DIR)
_ETC = os.path.join(_MODULE_DIR, "var", "etc")
_NGINX = os.path.join(_ETC, "origin.nginx.conf")
_CADDY = os.path.join(_ETC, "origin.Caddyfile")

_BINARY_EXTS = (".mwm", ".spa", ".sig")
_PLACEHOLDER_HOST = "maps.example.invalid"
_DOCROOT = "/var/www/street-pixels"


def _strip_hash_comments(text):
    lines = []
    for line in text.splitlines():
        lines.append(line.split("#", 1)[0])
    return "\n".join(lines)


class OriginConfigTest(unittest.TestCase):
    def test_snippets_exist(self):
        self.assertTrue(os.path.isfile(_NGINX))
        self.assertTrue(os.path.isfile(_CADDY))

    def test_nginx_gzip_off_range_no_debug(self):
        with open(_NGINX, encoding="utf-8") as f:
            raw = f.read()
        active = _strip_hash_comments(raw)
        self.assertIn("gzip off", active)
        self.assertNotIn("gzip on", active)
        self.assertNotIn("gzip_static on", active)
        self.assertIn(_PLACEHOLDER_HOST, active)
        self.assertIn(_DOCROOT, active)
        for ext in _BINARY_EXTS:
            self.assertIn(ext.lstrip("."), active)
        self.assertNotIn("enable-debug-routes", active)
        self.assertNotIn("/debug/inventory", active)
        self.assertNotIn("streifzug.app", active)
        self.assertNotIn("comaps.tech", active)
        self.assertNotIn("/spa/", active)

    def test_caddy_no_gzip_encode_no_debug(self):
        with open(_CADDY, encoding="utf-8") as f:
            raw = f.read()
        active = _strip_hash_comments(raw)
        self.assertNotIn("encode", active)
        self.assertIn("file_server", active)
        self.assertIn(_PLACEHOLDER_HOST, active)
        self.assertIn(_DOCROOT, active)
        self.assertNotIn("enable-debug-routes", active)
        self.assertNotIn("/debug/inventory", active)
        self.assertNotIn("streifzug.app", active)
        self.assertNotIn("comaps.tech", active)
        self.assertNotIn("/spa/", active)
        self.assertNotIn("browse", active)


if __name__ == "__main__":
    unittest.main()
