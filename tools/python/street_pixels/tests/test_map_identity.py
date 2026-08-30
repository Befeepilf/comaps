#!/usr/bin/env python3
"""Unit tests for street_pixels map identity (SP-101)."""

import json
import os
import re
import subprocess
import sys
import tempfile
import unittest
import urllib.error
from unittest import mock


_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_TOOLS_PYTHON = os.path.dirname(os.path.dirname(_THIS_DIR))
_REPO_ROOT = os.path.dirname(os.path.dirname(_TOOLS_PYTHON))
if _TOOLS_PYTHON not in sys.path:
    sys.path.insert(0, _TOOLS_PYTHON)

from street_pixels.map_identity import ACTION_COPY  # noqa: E402
from street_pixels.map_identity import ACTION_FETCH  # noqa: E402
from street_pixels.map_identity import ACTION_KEEP  # noqa: E402
from street_pixels.map_identity import ACTION_SKIP  # noqa: E402
from street_pixels.map_identity import EXAMPLE_PRIVATE_H  # noqa: E402
from street_pixels.map_identity import LOCKED_MAP_SERIES  # noqa: E402
from street_pixels.map_identity import MapIdentityError  # noqa: E402
from street_pixels.map_identity import PLACEHOLDER_MAP_ORIGIN  # noqa: E402
from street_pixels.map_identity import apply_world_bootstrap  # noqa: E402
from street_pixels.map_identity import comaps_map_host_in  # noqa: E402
from street_pixels.map_identity import configure_world  # noqa: E402
from street_pixels.map_identity import download_to_file  # noqa: E402
from street_pixels.map_identity import ed25519_public_key_hex  # noqa: E402
from street_pixels.map_identity import ensure_private_h  # noqa: E402
from street_pixels.map_identity import generate_ed25519_pem_pair  # noqa: E402
from street_pixels.map_identity import join_world_url  # noqa: E402
from street_pixels.map_identity import main as map_identity_main  # noqa: E402
from street_pixels.map_identity import resolve_world_bootstrap  # noqa: E402
from street_pixels.map_identity import sign_rawin  # noqa: E402
from street_pixels.map_identity import verify_rawin  # noqa: E402
from street_pixels.map_pipeline import COMAPS_MAP_HOSTS  # noqa: E402


SERIES = "2026.06.28"
VERSION = "260714"
CONFIGURE_SH = os.path.join(_REPO_ROOT, "configure.sh")
CMAKE_LISTS = os.path.join(_REPO_ROOT, "CMakeLists.txt")
FILE_PY = os.path.join(_TOOLS_PYTHON, "maps_generator", "utils", "file.py")
COMAPS_PUBLIC_HEX = "91c0a9f6aa182371f047e256ab46489211acc2b51b13197fbe8c94eaa9749c7b"


def _write_countries(path, series=SERIES, version=int(VERSION)):
    with open(path, "w", encoding="utf-8") as f:
        json.dump({"id": "Countries", "v": version, "map_series": series, "g": []}, f)


def _write_world(path, payload=b"world-bytes"):
    parent = os.path.dirname(path)
    if parent:
        os.makedirs(parent, exist_ok=True)
    with open(path, "wb") as f:
        f.write(payload)


class TemplateAuditTest(unittest.TestCase):
    def test_example_private_h_has_placeholder_origin_not_comaps_or_lan(self):
        self.assertTrue(os.path.isfile(EXAMPLE_PRIVATE_H))
        with open(EXAMPLE_PRIVATE_H, encoding="utf-8") as f:
            text = f.read()
        for host in COMAPS_MAP_HOSTS:
            self.assertNotIn(host, text)
        self.assertNotIn("192.168.", text)
        self.assertNotIn("comaps.app", text)
        self.assertNotIn("comaps.tech", text)
        self.assertIn(PLACEHOLDER_MAP_ORIGIN, text)
        self.assertIn('#define METASERVER_URL ""', text)
        self.assertIn('#define MAP_SERIES "{}"'.format(LOCKED_MAP_SERIES), text)
        hex_match = re.search(
            r'#define COUNTRIES_TXT_SIGNATURE_HEX "([0-9a-fA-F]+)"', text
        )
        self.assertIsNotNone(hex_match)
        self.assertEqual(64, len(hex_match.group(1)))
        self.assertNotEqual(
            COMAPS_PUBLIC_HEX,
            hex_match.group(1).lower(),
        )
        self.assertNotRegex(text, r'https?://10\.')
        self.assertNotRegex(text, r'https?://172\.(1[6-9]|2[0-9]|3[0-1])\.')

    def test_private_h_is_not_tracked_by_git(self):
        proc = subprocess.run(
            ["git", "-C", _REPO_ROOT, "ls-files", "--", "private.h"],
            capture_output=True,
            text=True,
            check=True,
        )
        self.assertEqual("", proc.stdout.strip())

    def test_ensure_private_h_copies_example_when_missing(self):
        with tempfile.TemporaryDirectory() as tmp:
            example_src = EXAMPLE_PRIVATE_H
            self.assertTrue(os.path.isfile(example_src))
            shutil_example = os.path.join(tmp, "private.h.street-pixels.example")
            with open(example_src, encoding="utf-8") as f:
                body = f.read()
            with open(shutil_example, "w", encoding="utf-8") as f:
                f.write(body)
            dest, copied = ensure_private_h(tmp)
            self.assertTrue(copied)
            self.assertEqual(os.path.join(tmp, "private.h"), dest)
            with open(dest, encoding="utf-8") as f:
                copied_text = f.read()
            self.assertEqual(body, copied_text)
            for host in COMAPS_MAP_HOSTS:
                self.assertNotIn(host, copied_text)
            self.assertNotIn(COMAPS_PUBLIC_HEX, copied_text.lower())
            dest2, copied2 = ensure_private_h(tmp)
            self.assertFalse(copied2)
            self.assertEqual(dest, dest2)

    def test_ensure_private_h_does_not_overwrite_existing(self):
        with tempfile.TemporaryDirectory() as tmp:
            with open(
                os.path.join(tmp, "private.h.street-pixels.example"),
                "w",
                encoding="utf-8",
            ) as f:
                f.write('#define DEFAULT_URLS_JSON R"([ "https://maps.example.invalid/" ])"\n')
            existing = os.path.join(tmp, "private.h")
            with open(existing, "w", encoding="utf-8") as f:
                f.write('#define DEFAULT_URLS_JSON R"([ "https://mapgen-fi-1.comaps.app/" ])"\n')
            dest, copied = ensure_private_h(tmp)
            self.assertFalse(copied)
            self.assertEqual(existing, dest)
            with open(existing, encoding="utf-8") as f:
                self.assertIn("mapgen-fi-1.comaps.app", f.read())

    def test_ensure_private_h_errors_when_example_missing(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(MapIdentityError) as ctx:
                ensure_private_h(tmp)
            self.assertIn("private.h.street-pixels.example", str(ctx.exception))


class ConfigureScriptTest(unittest.TestCase):
    def test_configure_sh_does_not_fetch_comaps_world(self):
        with open(CONFIGURE_SH, encoding="utf-8") as f:
            text = f.read()
        self.assertNotIn("https://mapgen-fi-1.comaps.app", text)
        self.assertNotIn("mapgen-fi-1.comaps.app/maps", text)
        for host in COMAPS_MAP_HOSTS:
            self.assertNotIn(host, text)
        self.assertIn("street_pixels.map_identity", text)
        self.assertIn("configure-world", text)
        self.assertIn("ensure-private-h", text)
        self.assertIn('export PYTHONPATH="$REPO_ROOT/tools/python', text)
        pythonpath_at = text.find("export PYTHONPATH=")
        module_at = text.find("python3 -m street_pixels.map_identity")
        self.assertGreater(pythonpath_at, 0)
        self.assertGreater(module_at, pythonpath_at)
        self.assertIn("STREET_PIXELS_MAPS_BASE_URL", text)
        self.assertIn("SKIP_MAP_DOWNLOAD", text)
        self.assertIn("STREET_PIXELS_LOCAL_WORLD", text)
        self.assertIn("STREET_PIXELS_WORLD_DIR", text)

    def test_cmake_seeds_private_h_from_example_when_missing(self):
        with open(CMAKE_LISTS, encoding="utf-8") as f:
            text = f.read()
        self.assertIn("private.h.street-pixels.example", text)
        self.assertIn('NOT EXISTS "${OMIM_ROOT}/private.h"', text)
        self.assertIn("COPYONLY", text)
        for host in COMAPS_MAP_HOSTS:
            self.assertNotIn(host, text)

    def test_map_identity_module_runs_with_configure_sh_pythonpath(self):
        env = os.environ.copy()
        extra = _TOOLS_PYTHON
        existing = env.get("PYTHONPATH", "")
        env["PYTHONPATH"] = extra if not existing else extra + os.pathsep + existing
        proc = subprocess.run(
            [
                sys.executable,
                "-m",
                "street_pixels.map_identity",
                "resolve",
                "--skip-map-download",
                "1",
                "--map-series",
                SERIES,
                "--mwm-version",
                VERSION,
            ],
            cwd=_REPO_ROOT,
            env=env,
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(0, proc.returncode, proc.stderr)
        plan = json.loads(proc.stdout)
        self.assertEqual(ACTION_SKIP, plan["action"])
        self.assertIsNone(plan["world_url"])

    def test_resolve_cli_refuses_comaps_maps_base_url(self):
        with tempfile.TemporaryDirectory() as tmp:
            countries = os.path.join(tmp, "countries.txt")
            _write_countries(countries)
            code = map_identity_main(
                [
                    "resolve",
                    "--countries",
                    countries,
                    "--data-dir",
                    tmp,
                    "--legacy-maps-base-url",
                    "https://mapgen-fi-1.comaps.app/maps/{}/{}".format(SERIES, VERSION),
                ]
            )
        self.assertEqual(1, code)

    def test_default_without_skip_or_origin_errors(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(MapIdentityError) as ctx:
                resolve_world_bootstrap(
                    data_dir=tmp,
                    map_series=SERIES,
                    mwm_version=VERSION,
                )
            message = str(ctx.exception)
            self.assertIn("SPD-087", message)
            self.assertIn("SKIP_MAP_DOWNLOAD", message)
            self.assertIn("STREET_PIXELS_MAPS_BASE_URL", message)
            self.assertNotIn("https://mapgen-fi-1.comaps.app", message)


class WorldBootstrapTest(unittest.TestCase):
    def test_skip_map_download(self):
        plan = resolve_world_bootstrap(
            skip_map_download="1",
            maps_base_url="https://mapgen-fi-1.comaps.app/",
            map_series=SERIES,
            mwm_version=VERSION,
        )
        self.assertEqual(ACTION_SKIP, plan["action"])
        self.assertIsNone(plan["world_url"])

    def test_existing_world_keeps_without_fetch(self):
        with tempfile.TemporaryDirectory() as tmp:
            _write_world(os.path.join(tmp, "world_mwm", VERSION, "World.mwm"))
            plan = resolve_world_bootstrap(
                data_dir=tmp,
                map_series=SERIES,
                mwm_version=VERSION,
                maps_base_url="https://mapgen-fi-1.comaps.app/",
            )
            self.assertEqual(ACTION_KEEP, plan["action"])
            self.assertIsNone(plan["world_url"])

    def test_local_world_copy(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = os.path.join(tmp, "World.mwm")
            _write_world(src, b"from-operator")
            data_dir = os.path.join(tmp, "data")
            os.makedirs(data_dir)
            plan = resolve_world_bootstrap(
                local_world=src,
                data_dir=data_dir,
                map_series=SERIES,
                mwm_version=VERSION,
            )
            self.assertEqual(ACTION_COPY, plan["action"])
            self.assertEqual(src, plan["world_source"])
            apply_world_bootstrap(plan, data_dir)
            dest = os.path.join(data_dir, "world_mwm", VERSION, "World.mwm")
            with open(dest, "rb") as f:
                self.assertEqual(b"from-operator", f.read())
            self.assertTrue(os.path.islink(os.path.join(data_dir, "World.mwm")))

    def test_world_dir_copy_omits_missing_coasts(self):
        with tempfile.TemporaryDirectory() as tmp:
            world_dir = os.path.join(tmp, "mwms")
            _write_world(os.path.join(world_dir, "World.mwm"), b"w")
            data_dir = os.path.join(tmp, "data")
            os.makedirs(data_dir)
            plan = resolve_world_bootstrap(
                world_dir=world_dir,
                data_dir=data_dir,
                map_series=SERIES,
                mwm_version=VERSION,
            )
            self.assertEqual(ACTION_COPY, plan["action"])
            self.assertIsNone(plan["coasts_source"])
            apply_world_bootstrap(plan, data_dir)
            self.assertTrue(
                os.path.isfile(os.path.join(data_dir, "world_mwm", VERSION, "World.mwm"))
            )
            self.assertFalse(
                os.path.isfile(
                    os.path.join(data_dir, "world_mwm", VERSION, "WorldCoasts.mwm")
                )
            )

    def test_fetch_from_placeholder_origin_not_comaps(self):
        recorded = []

        def fake_download(url, dest):
            recorded.append(url)
            self.assertNotIn("comaps", url.lower())
            parent = os.path.dirname(dest)
            os.makedirs(parent, exist_ok=True)
            if url.endswith("WorldCoasts.mwm"):
                return False
            with open(dest, "wb") as f:
                f.write(b"fetched-world")
            return True

        with tempfile.TemporaryDirectory() as tmp:
            plan = resolve_world_bootstrap(
                maps_base_url=PLACEHOLDER_MAP_ORIGIN,
                data_dir=tmp,
                map_series=SERIES,
                mwm_version=VERSION,
            )
            self.assertEqual(ACTION_FETCH, plan["action"])
            self.assertTrue(plan["world_url"].startswith(PLACEHOLDER_MAP_ORIGIN))
            self.assertIn("/maps/{}/{}/World.mwm".format(SERIES, VERSION), plan["world_url"])
            apply_world_bootstrap(plan, tmp, downloader=fake_download)
            with open(os.path.join(tmp, "world_mwm", VERSION, "World.mwm"), "rb") as f:
                self.assertEqual(b"fetched-world", f.read())
            self.assertFalse(
                os.path.isfile(os.path.join(tmp, "world_mwm", VERSION, "WorldCoasts.mwm"))
            )
        self.assertEqual(2, len(recorded))
        for url in recorded:
            for host in COMAPS_MAP_HOSTS:
                self.assertNotIn(host, url)

    def test_every_comaps_host_refused_as_maps_base_url(self):
        for host in COMAPS_MAP_HOSTS:
            with self.subTest(host=host):
                with self.assertRaises(MapIdentityError) as ctx:
                    resolve_world_bootstrap(
                        maps_base_url="https://{}/".format(host),
                        map_series=SERIES,
                        mwm_version=VERSION,
                    )
                self.assertIn("CoMaps", str(ctx.exception))
                self.assertIsNotNone(comaps_map_host_in("https://{}/x".format(host)))

    def test_legacy_mapgen_fi_1_maps_base_url_refused(self):
        with self.assertRaises(MapIdentityError) as ctx:
            resolve_world_bootstrap(
                legacy_maps_base_url="https://mapgen-fi-1.comaps.app/maps/{}/{}".format(
                    SERIES, VERSION
                ),
                map_series=SERIES,
                mwm_version=VERSION,
            )
        self.assertIn("mapgen-fi-1.comaps.app", str(ctx.exception))

    def test_lan_maps_base_url_refused(self):
        with self.assertRaises(MapIdentityError) as ctx:
            resolve_world_bootstrap(
                maps_base_url="http://192.168.1.10/maps",
                map_series=SERIES,
                mwm_version=VERSION,
            )
        self.assertIn("HTTPS", str(ctx.exception))
        with self.assertRaises(MapIdentityError):
            resolve_world_bootstrap(
                maps_base_url="https://10.0.0.5/",
                map_series=SERIES,
                mwm_version=VERSION,
            )

    def test_configure_world_skip_does_not_call_downloader(self):
        downloader = mock.Mock()
        with tempfile.TemporaryDirectory() as tmp:
            countries = os.path.join(tmp, "countries.txt")
            _write_countries(countries)
            plan = configure_world(
                data_dir=tmp,
                countries_path=countries,
                skip_map_download="1",
                downloader=downloader,
            )
        self.assertEqual(ACTION_SKIP, plan["action"])
        downloader.assert_not_called()

    def test_fetch_downloader_never_receives_comaps_url(self):
        def boom(url, dest):
            self.fail("must not fetch {!r}".format(url))

        with tempfile.TemporaryDirectory() as tmp:
            countries = os.path.join(tmp, "countries.txt")
            _write_countries(countries)
            with self.assertRaises(MapIdentityError):
                configure_world(
                    data_dir=tmp,
                    countries_path=countries,
                    maps_base_url="https://cdn-us-1.comaps.app/",
                    downloader=boom,
                )

    def test_join_world_url_uses_spd035_layout(self):
        url = join_world_url(PLACEHOLDER_MAP_ORIGIN, SERIES, VERSION, "World.mwm")
        self.assertEqual(
            "https://maps.example.invalid/maps/{}/{}/World.mwm".format(SERIES, VERSION),
            url,
        )


class SignatureRoundTripTest(unittest.TestCase):
    def test_sign_rawin_matches_maps_generator_sign_file_argv(self):
        with open(FILE_PY, encoding="utf-8") as f:
            text = f.read()
        self.assertIn('"pkeyutl", "-sign"', text)
        self.assertIn('"-rawin", "-in"', text)
        self.assertIn('"pkeyutl", "-verify"', text)
        self.assertIn('"-sigfile"', text)
        self.assertIn("def sign_file(", text)
        self.assertIn("def verify_file(", text)

    def test_throwaway_ed25519_sign_verify_and_public_hex(self):
        with tempfile.TemporaryDirectory() as tmp:
            secret = os.path.join(tmp, "secret.pem")
            public = os.path.join(tmp, "public.pem")
            generate_ed25519_pem_pair(secret, public)
            countries = os.path.join(tmp, "countries.txt")
            body = '{"id":"Countries","v":260714,"map_series":"2026.06.28"}\n'
            with open(countries, "w", encoding="utf-8") as f:
                f.write(body)
            sig = sign_rawin(countries, secret)
            self.assertTrue(os.path.isfile(sig))
            self.assertTrue(verify_rawin(countries, sig, public))
            with open(countries, "a", encoding="utf-8") as f:
                f.write("tamper")
            self.assertFalse(verify_rawin(countries, sig, public))
            hex_key = ed25519_public_key_hex(public)
            self.assertEqual(64, len(hex_key))
            self.assertRegex(hex_key, r"^[0-9a-f]{64}$")
            with open(public, "r", encoding="utf-8") as f:
                pem = f.read()
            self.assertIn("BEGIN PUBLIC KEY", pem)
            with open(secret, "r", encoding="utf-8") as f:
                secret_pem = f.read()
            self.assertIn("BEGIN PRIVATE KEY", secret_pem)


class DownloadGuardTest(unittest.TestCase):
    def test_download_to_file_refuses_comaps_before_http(self):
        with mock.patch("urllib.request.urlretrieve") as retrieve:
            with self.assertRaises(MapIdentityError) as ctx:
                download_to_file(
                    "https://mapgen-fi-1.comaps.app/maps/{}/{}/World.mwm".format(
                        SERIES, VERSION
                    ),
                    os.path.join("unused", "World.mwm"),
                )
            retrieve.assert_not_called()
            self.assertIn("CoMaps", str(ctx.exception))

    def test_download_404_returns_false(self):
        err = urllib.error.HTTPError(
            PLACEHOLDER_MAP_ORIGIN + "WorldCoasts.mwm",
            404,
            "Not Found",
            hdrs=mock.Mock(),
            fp=None,
        )
        with tempfile.TemporaryDirectory() as tmp:
            dest = os.path.join(tmp, "WorldCoasts.mwm")
            with mock.patch("urllib.request.urlretrieve", side_effect=err):
                self.assertFalse(
                    download_to_file(
                        PLACEHOLDER_MAP_ORIGIN
                        + "maps/{}/{}/WorldCoasts.mwm".format(SERIES, VERSION),
                        dest,
                    )
                )
            self.assertFalse(os.path.exists(dest))


if __name__ == "__main__":
    unittest.main()
