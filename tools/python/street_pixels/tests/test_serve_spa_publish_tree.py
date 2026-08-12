#!/usr/bin/env python3
"""Unit tests for serve_spa_publish_tree (SP-051)."""

import json
import os
import sys
import tempfile
import threading
import unittest
from http.client import HTTPConnection
from http.server import ThreadingHTTPServer
from urllib.error import HTTPError
from urllib.request import Request
from urllib.request import urlopen


_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_TOOLS_PYTHON = os.path.dirname(os.path.dirname(_THIS_DIR))
if _TOOLS_PYTHON not in sys.path:
    sys.path.insert(0, _TOOLS_PYTHON)

from street_pixels.serve_spa_publish_tree import build_custom_maps_url  # noqa: E402
from street_pixels.serve_spa_publish_tree import make_handler  # noqa: E402
from street_pixels.serve_spa_publish_tree import parse_range_header  # noqa: E402
from street_pixels.serve_spa_publish_tree import resolve_under_root  # noqa: E402


class ParseRangeTest(unittest.TestCase):
    def test_full_absent(self):
        self.assertIsNone(parse_range_header(None, 100))

    def test_closed(self):
        self.assertEqual((10, 19), parse_range_header("bytes=10-19", 100))

    def test_open_end(self):
        self.assertEqual((50, 99), parse_range_header("bytes=50-", 100))

    def test_suffix(self):
        self.assertEqual((90, 99), parse_range_header("bytes=-10", 100))

    def test_unsatisfiable(self):
        with self.assertRaises(ValueError):
            parse_range_header("bytes=100-110", 100)


class ResolveUnderRootTest(unittest.TestCase):
    def test_rejects_traversal(self):
        with tempfile.TemporaryDirectory() as tmp:
            secret = os.path.join(os.path.dirname(tmp), "outside-secret")
            try:
                with open(secret, "w") as f:
                    f.write("nope")
                self.assertIsNone(
                    resolve_under_root(tmp, "/../" + os.path.basename(secret))
                )
            finally:
                if os.path.isfile(secret):
                    os.remove(secret)

    def test_resolves_nested(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "maps", "a", "1", "x.spa")
            os.makedirs(os.path.dirname(path))
            with open(path, "wb") as f:
                f.write(b"spa")
            got = resolve_under_root(tmp, "/maps/a/1/x.spa")
            self.assertEqual(os.path.realpath(path), got)


class ServeSpaPublishTreeTest(unittest.TestCase):
    SERIES = "2026.06.28"
    DATA_V = 260714

    def _tree(self, tmp):
        leaf = "Finland_Helsinki"
        vdir = os.path.join(tmp, "maps", self.SERIES, str(self.DATA_V))
        os.makedirs(vdir)
        os.makedirs(os.path.join(tmp, "meta"))
        spa = b"spa-bytes-payload-0123456789"
        mwm = b"mwm-bytes-ABCDEFGHIJKLMNOP"
        with open(os.path.join(vdir, "{}.spa".format(leaf)), "wb") as f:
            f.write(spa)
        with open(os.path.join(vdir, "{}.mwm".format(leaf)), "wb") as f:
            f.write(mwm)
        countries = {"id": "Countries", "v": self.DATA_V, "map_series": self.SERIES}
        with open(os.path.join(vdir, "countries.txt"), "w") as f:
            json.dump(countries, f)
        maps_json = {
            "map-series": {self.SERIES: {"latest": self.DATA_V, "status": "active"}}
        }
        with open(os.path.join(tmp, "meta", "maps.json"), "w") as f:
            json.dump(maps_json, f)
        inv = {
            "map_series": self.SERIES,
            "publish_version": self.DATA_V,
            "leaves": [leaf],
        }
        with open(os.path.join(tmp, "inventory.json"), "w") as f:
            json.dump(inv, f)
        return leaf, spa, mwm

    def _serve(self, root, enable_debug=False):
        handler = make_handler(root, enable_debug, log_access=False)
        server = ThreadingHTTPServer(("127.0.0.1", 0), handler)
        port = server.server_address[1]
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(server.shutdown)
        self.addCleanup(server.server_close)
        return port

    def test_get_spa_exact_bytes(self):
        with tempfile.TemporaryDirectory() as tmp:
            leaf, spa, _mwm = self._tree(tmp)
            port = self._serve(tmp)
            url = "http://127.0.0.1:{}/maps/{}/{}/{}.spa".format(
                port, self.SERIES, self.DATA_V, leaf
            )
            with urlopen(url, timeout=5) as resp:
                self.assertEqual(200, resp.status)
                self.assertEqual(spa, resp.read())
                self.assertEqual(
                    "application/octet-stream",
                    resp.headers.get("Content-Type"),
                )

    def test_range_partial_content(self):
        with tempfile.TemporaryDirectory() as tmp:
            leaf, spa, _mwm = self._tree(tmp)
            port = self._serve(tmp)
            url = "http://127.0.0.1:{}/maps/{}/{}/{}.spa".format(
                port, self.SERIES, self.DATA_V, leaf
            )
            req = Request(url, headers={"Range": "bytes=0-3"})
            with urlopen(req, timeout=5) as resp:
                self.assertEqual(206, resp.status)
                self.assertEqual(spa[0:4], resp.read())
                self.assertEqual(
                    "bytes 0-3/{}".format(len(spa)),
                    resp.headers.get("Content-Range"),
                )

    def test_wrong_path_404(self):
        with tempfile.TemporaryDirectory() as tmp:
            self._tree(tmp)
            port = self._serve(tmp)
            url = "http://127.0.0.1:{}/maps/{}/{}/missing.spa".format(
                port, self.SERIES, self.DATA_V
            )
            with self.assertRaises(HTTPError) as ctx:
                urlopen(url, timeout=5)
            self.assertEqual(404, ctx.exception.code)

    def test_debug_inventory_off_by_default(self):
        with tempfile.TemporaryDirectory() as tmp:
            self._tree(tmp)
            port = self._serve(tmp, enable_debug=False)
            url = "http://127.0.0.1:{}/debug/inventory".format(port)
            with self.assertRaises(HTTPError) as ctx:
                urlopen(url, timeout=5)
            self.assertEqual(404, ctx.exception.code)

    def test_debug_inventory_opt_in(self):
        with tempfile.TemporaryDirectory() as tmp:
            self._tree(tmp)
            port = self._serve(tmp, enable_debug=True)
            url = "http://127.0.0.1:{}/debug/inventory".format(port)
            with urlopen(url, timeout=5) as resp:
                self.assertEqual(200, resp.status)
                payload = json.loads(resp.read().decode("utf-8"))
                self.assertEqual(self.SERIES, payload["map_series"])

    def test_health(self):
        with tempfile.TemporaryDirectory() as tmp:
            self._tree(tmp)
            port = self._serve(tmp)
            url = "http://127.0.0.1:{}/health".format(port)
            with urlopen(url, timeout=5) as resp:
                self.assertEqual(200, resp.status)
                payload = json.loads(resp.read().decode("utf-8"))
                self.assertTrue(payload["ok"])
                self.assertEqual(self.SERIES, payload["map_series"])
                self.assertEqual(self.DATA_V, payload["data_version"])
                self.assertEqual(1, payload["spa_leaf_count"])

    def test_custom_maps_url_banner(self):
        url = build_custom_maps_url("127.0.0.1", 8080, "127.0.0.1")
        self.assertEqual("http://127.0.0.1:8080/", url)

    def test_meta_maps_json(self):
        with tempfile.TemporaryDirectory() as tmp:
            self._tree(tmp)
            port = self._serve(tmp)
            url = "http://127.0.0.1:{}/meta/maps.json".format(port)
            with urlopen(url, timeout=5) as resp:
                self.assertEqual(200, resp.status)
                payload = json.loads(resp.read().decode("utf-8"))
                self.assertEqual(
                    self.DATA_V,
                    payload["map-series"][self.SERIES]["latest"],
                )

    def test_inventory_json_not_served_without_debug(self):
        with tempfile.TemporaryDirectory() as tmp:
            self._tree(tmp)
            port = self._serve(tmp, enable_debug=False)
            url = "http://127.0.0.1:{}/inventory.json".format(port)
            with self.assertRaises(HTTPError) as ctx:
                urlopen(url, timeout=5)
            self.assertEqual(404, ctx.exception.code)

    def test_head_debug_inventory_when_enabled(self):
        with tempfile.TemporaryDirectory() as tmp:
            self._tree(tmp)
            port = self._serve(tmp, enable_debug=True)
            conn = HTTPConnection("127.0.0.1", port, timeout=5)
            try:
                conn.request("HEAD", "/debug/inventory")
                resp = conn.getresponse()
                self.assertEqual(200, resp.status)
                self.assertGreater(int(resp.getheader("Content-Length")), 0)
                self.assertEqual(b"", resp.read())
            finally:
                conn.close()

    def test_url_encoded_leaf_with_space(self):
        with tempfile.TemporaryDirectory() as tmp:
            leaf = "Finland_Southern Finland_Helsinki"
            vdir = os.path.join(tmp, "maps", self.SERIES, str(self.DATA_V))
            os.makedirs(vdir)
            os.makedirs(os.path.join(tmp, "meta"))
            spa = b"helsinki-spa"
            with open(os.path.join(vdir, "{}.spa".format(leaf)), "wb") as f:
                f.write(spa)
            with open(os.path.join(tmp, "meta", "maps.json"), "w") as f:
                json.dump(
                    {
                        "map-series": {
                            self.SERIES: {"latest": self.DATA_V, "status": "active"}
                        }
                    },
                    f,
                )
            with open(os.path.join(tmp, "inventory.json"), "w") as f:
                json.dump(
                    {
                        "map_series": self.SERIES,
                        "publish_version": self.DATA_V,
                        "leaves": [{"id": leaf, "advertised": True, "spa_bytes": len(spa)}],
                    },
                    f,
                )
            port = self._serve(tmp)
            encoded = "Finland_Southern%20Finland_Helsinki.spa"
            url = "http://127.0.0.1:{}/maps/{}/{}/{}".format(
                port, self.SERIES, self.DATA_V, encoded
            )
            with urlopen(url, timeout=5) as resp:
                self.assertEqual(200, resp.status)
                self.assertEqual(spa, resp.read())


if __name__ == "__main__":
    unittest.main()
