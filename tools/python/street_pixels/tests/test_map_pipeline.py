#!/usr/bin/env python3
"""Unit tests for street_pixels map_pipeline (SP-100)."""

import os
import sys
import tempfile
import unittest
from unittest import mock


_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_TOOLS_PYTHON = os.path.dirname(os.path.dirname(_THIS_DIR))
if _TOOLS_PYTHON not in sys.path:
    sys.path.insert(0, _TOOLS_PYTHON)

from street_pixels.map_pipeline import COMAPS_MAP_HOSTS  # noqa: E402
from street_pixels.map_pipeline import CORE_STAGES  # noqa: E402
from street_pixels.map_pipeline import DEFAULT_COUNTRIES  # noqa: E402
from street_pixels.map_pipeline import EXTRACT_RINGS_SCRIPT  # noqa: E402
from street_pixels.map_pipeline import MAPGEN_SKIP_STAGE_NAMES  # noqa: E402
from street_pixels.map_pipeline import MapPipelineError  # noqa: E402
from street_pixels.map_pipeline import STAGE_ASSEMBLE  # noqa: E402
from street_pixels.map_pipeline import STAGE_MAPGEN  # noqa: E402
from street_pixels.map_pipeline import STAGE_PIX_DERIVE  # noqa: E402
from street_pixels.map_pipeline import STAGE_RINGS  # noqa: E402
from street_pixels.map_pipeline import STAGE_RSYNC  # noqa: E402
from street_pixels.map_pipeline import STAGE_SPA_EMIT  # noqa: E402
from street_pixels.map_pipeline import build_mapgen_argv  # noqa: E402
from street_pixels.map_pipeline import build_pix_derive_argv  # noqa: E402
from street_pixels.map_pipeline import build_plan  # noqa: E402
from street_pixels.map_pipeline import build_rings_argv  # noqa: E402
from street_pixels.map_pipeline import build_spa_emit_argv  # noqa: E402
from street_pixels.map_pipeline import ensure_planet_md5_url  # noqa: E402
from street_pixels.map_pipeline import expand_countries  # noqa: E402
from street_pixels.map_pipeline import load_default_ini_text  # noqa: E402
from street_pixels.map_pipeline import pipeline_stage_names  # noqa: E402
from street_pixels.map_pipeline import run_map_pipeline  # noqa: E402
from street_pixels.map_pipeline import run_mapgen  # noqa: E402


def _touch_poly(directory, leaf_id):
    path = os.path.join(directory, "{}.poly".format(leaf_id))
    with open(path, "w") as f:
        f.write("dummy\n")
    return path


def _finland_borders(tmp):
    borders = os.path.join(tmp, "borders")
    os.makedirs(borders)
    for leaf in (
        "Finland_Southern Finland_Helsinki",
        "Finland_Northern Finland",
        "Finland_Eastern Finland_North",
        "Finland_Eastern Finland_South",
        "Finland_Southern Finland_Lappeenranta",
        "Finland_Southern Finland_West",
        "Finland_Western Finland_Jyvaskyla",
        "Finland_Western Finland_Tampere",
    ):
        _touch_poly(borders, leaf)
    return borders


class StageGraphTest(unittest.TestCase):
    def test_core_order_mapgen_to_assemble(self):
        stages = pipeline_stage_names()
        self.assertEqual(
            (
                STAGE_MAPGEN,
                STAGE_PIX_DERIVE,
                STAGE_RINGS,
                STAGE_SPA_EMIT,
                STAGE_ASSEMBLE,
            ),
            stages,
        )
        self.assertEqual(CORE_STAGES, stages)
        self.assertNotIn(STAGE_RSYNC, stages)

    def test_rsync_is_last_when_dest_set(self):
        stages = pipeline_stage_names(rsync_dest="user@host:/var/www/maps")
        self.assertEqual(
            (
                STAGE_MAPGEN,
                STAGE_PIX_DERIVE,
                STAGE_RINGS,
                STAGE_SPA_EMIT,
                STAGE_ASSEMBLE,
                STAGE_RSYNC,
            ),
            stages,
        )
        self.assertEqual(STAGE_RSYNC, stages[-1])


class DryRunNoNetworkTest(unittest.TestCase):
    def test_dry_run_does_not_call_urllib_or_maps_generator(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            out = os.path.join(tmp, "out")
            with mock.patch("urllib.request.urlopen") as urlopen, mock.patch(
                "urllib.request.Request"
            ) as request, mock.patch("subprocess.run") as sub_run, mock.patch(
                "subprocess.Popen"
            ) as popen, mock.patch(
                "street_pixels.map_pipeline.run_command"
            ) as run_command:
                plan = run_map_pipeline(
                    pbf="file:///tmp/finland.osm.pbf",
                    out=out,
                    countries=DEFAULT_COUNTRIES,
                    borders_dir=borders,
                    dry_run=True,
                )
            urlopen.assert_not_called()
            request.assert_not_called()
            sub_run.assert_not_called()
            popen.assert_not_called()
            run_command.assert_not_called()
            self.assertTrue(plan["dry_run"])
            self.assertEqual(list(CORE_STAGES), plan["stages"])
            self.assertFalse(os.path.exists(out))
            self.assertFalse(os.path.exists(plan["work_dir"]))
            self.assertFalse(os.path.exists(plan["out"]))
            self.assertNotIn("osmium", sys.modules)
            self.assertNotIn("extract_admin_place_polygons", sys.modules)


class DefaultConfigDenylistTest(unittest.TestCase):
    def test_default_ini_has_no_comaps_map_hosts(self):
        text = load_default_ini_text()
        for host in COMAPS_MAP_HOSTS:
            self.assertNotIn(host, text)
        self.assertNotIn("cdn.organicmaps.app", text)
        self.assertIn("NODE_STORAGE: map", text)
        self.assertIn("THREADS_COUNT: 4", text)

    def test_generated_plan_ini_has_no_comaps_map_hosts(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="file:///tmp/finland.osm.pbf",
                out=os.path.join(tmp, "out"),
                borders_dir=borders,
                dry_run=True,
            )
            for host in COMAPS_MAP_HOSTS:
                self.assertNotIn(host, plan["ini_text"])
            self.assertNotIn("cdn.organicmaps.app", plan["ini_text"])
            self.assertIn("NODE_STORAGE: map", plan["ini_text"])


class OriginDenylistTest(unittest.TestCase):
    def test_comaps_host_in_pbf_refused_without_allow(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            with self.assertRaises(MapPipelineError) as ctx:
                build_plan(
                    pbf="https://cdn-us-1.comaps.app/maps/World.mwm",
                    out=os.path.join(tmp, "out"),
                    borders_dir=borders,
                    dry_run=True,
                )
            self.assertIn("cdn-us-1.comaps.app", str(ctx.exception))

    def test_cdn_base_refused_without_allow(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            with self.assertRaises(MapPipelineError) as ctx:
                build_plan(
                    pbf="file:///tmp/finland.osm.pbf",
                    out=os.path.join(tmp, "out"),
                    borders_dir=borders,
                    cdn_base=["https://maps.example.test/"],
                    dry_run=True,
                )
            self.assertIn("--cdn-base", str(ctx.exception))

    def test_comaps_cdn_base_refused_without_allow(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            with self.assertRaises(MapPipelineError):
                build_plan(
                    pbf="file:///tmp/finland.osm.pbf",
                    out=os.path.join(tmp, "out"),
                    borders_dir=borders,
                    cdn_base=["https://cdn.comaps.app/"],
                    dry_run=True,
                )

    def test_allow_flag_permits_cdn_base(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="file:///tmp/finland.osm.pbf",
                out=os.path.join(tmp, "out"),
                borders_dir=borders,
                cdn_base=["https://cdn.comaps.app/"],
                allow_comaps_origin=True,
                dry_run=True,
            )
            self.assertTrue(plan["dry_run"])


class SkipCoastPreflightTest(unittest.TestCase):
    def test_skip_coast_with_world_errors(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            with self.assertRaises(MapPipelineError) as ctx:
                build_plan(
                    pbf="file:///tmp/finland.osm.pbf",
                    out=os.path.join(tmp, "out"),
                    countries="World,Finland_*",
                    borders_dir=borders,
                    skip_coast=True,
                    dry_run=True,
                )
            self.assertIn("World", str(ctx.exception))
            self.assertIn("skip-coast", str(ctx.exception).lower().replace("_", "-"))

    def test_skip_coast_without_world_allowed_omits_worldcoasts(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="file:///tmp/finland.osm.pbf",
                out=os.path.join(tmp, "out"),
                countries="Finland_*",
                borders_dir=borders,
                skip_coast=True,
                dry_run=True,
            )
            self.assertNotIn("World", plan["countries"])
            self.assertNotIn("WorldCoasts", plan["countries"])
            self.assertTrue(plan["omit_world_coasts"])
            self.assertIn("Coastline", plan["mapgen_skip"])
            self.assertGreaterEqual(len(plan["countries"]), 8)

    def test_default_includes_world_omits_worldcoasts_does_not_skip_coast(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="file:///tmp/finland.osm.pbf",
                out=os.path.join(tmp, "out"),
                countries=DEFAULT_COUNTRIES,
                borders_dir=borders,
                dry_run=True,
            )
            self.assertIn("World", plan["countries"])
            self.assertNotIn("WorldCoasts", plan["countries"])
            self.assertNotIn("Coastline", plan["mapgen_skip"])
            self.assertTrue(plan["omit_world_coasts"])


class FromStageTest(unittest.TestCase):
    def test_from_stage_skips_earlier_stages(self):
        stages = pipeline_stage_names(from_stage=STAGE_SPA_EMIT)
        self.assertEqual((STAGE_SPA_EMIT, STAGE_ASSEMBLE), stages)
        self.assertNotIn(STAGE_MAPGEN, stages)
        self.assertNotIn(STAGE_PIX_DERIVE, stages)
        self.assertNotIn(STAGE_RINGS, stages)

    def test_from_stage_pix_derive_keeps_later(self):
        stages = pipeline_stage_names(from_stage=STAGE_PIX_DERIVE)
        self.assertEqual(
            (
                STAGE_PIX_DERIVE,
                STAGE_RINGS,
                STAGE_SPA_EMIT,
                STAGE_ASSEMBLE,
            ),
            stages,
        )

    def test_from_stage_in_plan(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="file:///tmp/finland.osm.pbf",
                out=os.path.join(tmp, "out"),
                borders_dir=borders,
                from_stage=STAGE_SPA_EMIT,
                dry_run=True,
            )
            self.assertEqual([STAGE_SPA_EMIT, STAGE_ASSEMBLE], plan["stages"])


class ExpandCountriesTest(unittest.TestCase):
    def test_finland_glob_and_world(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            expanded = expand_countries("World,Finland_*", borders)
            self.assertIn("World", expanded)
            self.assertNotIn("WorldCoasts", expanded)
            self.assertEqual(9, len(expanded))
            self.assertNotIn("W", expanded)
            self.assertNotIn("o", expanded)
            self.assertNotIn("r", expanded)
            self.assertNotIn("l", expanded)
            self.assertNotIn("d", expanded)
            self.assertNotIn("F", expanded)


class WorldCoastsExplicitTest(unittest.TestCase):
    def test_worldcoasts_only_when_named(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            omitted = expand_countries("World*", borders)
            self.assertIn("World", omitted)
            self.assertNotIn("WorldCoasts", omitted)
            explicit = expand_countries("WorldCoasts,Finland_*", borders)
            self.assertIn("WorldCoasts", explicit)


class MapgenSkipTokenTest(unittest.TestCase):
    def test_skip_tokens_match_stage_declaration_classes(self):
        decl = os.path.join(
            _TOOLS_PYTHON, "maps_generator", "generator", "stages_declaration.py"
        )
        with open(decl, "r", encoding="utf-8") as f:
            text = f.read()
        for token in MAPGEN_SKIP_STAGE_NAMES:
            self.assertIn("class Stage{}(".format(token), text)


class OriginDenylistExtraUrlTest(unittest.TestCase):
    def test_each_denylisted_host_refused_in_hotels_url(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            out = os.path.join(tmp, "out")
            for host in COMAPS_MAP_HOSTS:
                with self.subTest(host=host):
                    with self.assertRaises(MapPipelineError) as ctx:
                        build_plan(
                            pbf="file:///tmp/finland.osm.pbf",
                            out=out,
                            borders_dir=borders,
                            hotels_url="https://{}/hotels.csv".format(host),
                            dry_run=True,
                        )
                    self.assertIn(host, str(ctx.exception))

    def test_unlisted_comaps_app_subdomain_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            with self.assertRaises(MapPipelineError) as ctx:
                build_plan(
                    pbf="https://cdn-eu-1.comaps.app/maps/World.mwm",
                    out=os.path.join(tmp, "out"),
                    borders_dir=borders,
                    dry_run=True,
                )
            self.assertIn("comaps.app", str(ctx.exception))

    def test_path_substring_cdn_comaps_app_is_not_a_host_match(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            with self.assertRaises(MapPipelineError) as ctx:
                build_plan(
                    pbf="https://example.test/cdn.comaps.app/extract.osm.pbf",
                    out=os.path.join(tmp, "out"),
                    borders_dir=borders,
                    dry_run=True,
                )
            self.assertIn("example.test", str(ctx.exception))
            self.assertNotIn("CoMaps map host", str(ctx.exception))

    def test_geofabrik_https_pbf_allowed(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="https://download.geofabrik.de/europe/finland-latest.osm.pbf",
                out=os.path.join(tmp, "out"),
                borders_dir=borders,
                dry_run=True,
            )
            self.assertIn("geofabrik.de", plan["pbf_url"])

    def test_planet_osm_https_pbf_allowed(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="https://planet.openstreetmap.org/pbf/planet-latest.osm.pbf",
                out=os.path.join(tmp, "out"),
                borders_dir=borders,
                dry_run=True,
            )
            self.assertIn("planet.openstreetmap.org", plan["pbf_url"])

    def test_random_https_pbf_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            with self.assertRaises(MapPipelineError) as ctx:
                build_plan(
                    pbf="https://maps.example.test/finland.osm.pbf",
                    out=os.path.join(tmp, "out"),
                    borders_dir=borders,
                    dry_run=True,
                )
            self.assertIn("maps.example.test", str(ctx.exception))


class ProductionFlagTest(unittest.TestCase):
    def test_srtm_path_does_not_set_production(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            srtm = os.path.join(tmp, "srtm")
            os.makedirs(srtm)
            plan = build_plan(
                pbf="file:///tmp/finland.osm.pbf",
                out=os.path.join(tmp, "out"),
                borders_dir=borders,
                srtm_path=srtm,
                dry_run=True,
            )
            self.assertFalse(plan["mapgen_production"])
            self.assertNotIn("Srtm", plan["mapgen_skip"])
            self.assertNotIn("--production", build_mapgen_argv(plan))

    def test_hotels_url_sets_production(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="file:///tmp/finland.osm.pbf",
                out=os.path.join(tmp, "out"),
                borders_dir=borders,
                hotels_url="https://download.geofabrik.de/hotels.csv",
                dry_run=True,
            )
            self.assertTrue(plan["mapgen_production"])
            self.assertIn("--production", build_mapgen_argv(plan))


class FromStageRuntimeTest(unittest.TestCase):
    def test_from_stage_does_not_run_earlier_runners(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            out = os.path.join(tmp, "out")
            with mock.patch(
                "street_pixels.map_pipeline.run_mapgen"
            ) as mapgen, mock.patch(
                "street_pixels.map_pipeline.run_pix_derive"
            ) as pix, mock.patch(
                "street_pixels.map_pipeline.run_rings"
            ) as rings, mock.patch(
                "street_pixels.map_pipeline.run_spa_emit"
            ) as spa, mock.patch(
                "street_pixels.map_pipeline.run_assemble"
            ) as assemble:
                run_map_pipeline(
                    pbf="file:///tmp/finland.osm.pbf",
                    out=out,
                    borders_dir=borders,
                    from_stage=STAGE_SPA_EMIT,
                    spa_emit_bin="/usr/bin/true",
                    dry_run=False,
                )
            mapgen.assert_not_called()
            pix.assert_not_called()
            rings.assert_not_called()
            spa.assert_called_once()
            assemble.assert_called_once()
            self.assertFalse(os.path.isdir(out))


class CommandConstructionTest(unittest.TestCase):
    def test_pix_derive_argv_uses_mwm_dir_not_world_leaf(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="file:///tmp/finland.osm.pbf",
                out=os.path.join(tmp, "out"),
                borders_dir=borders,
                dry_run=True,
            )
            runtime = {
                "mwm_dir": os.path.join(tmp, "mwm"),
                "data_version": 260728,
            }
            argv = build_pix_derive_argv(plan, runtime)
            self.assertIn("--mwm_dir", argv)
            self.assertNotIn("World.mwm", argv)
            self.assertNotIn(os.path.join(runtime["mwm_dir"], "World.mwm"), argv)

    def test_spa_emit_argv_has_pix_borders_iso(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="file:///tmp/finland.osm.pbf",
                out=os.path.join(tmp, "out"),
                borders_dir=borders,
                dry_run=True,
            )
            runtime = {"data_version": 260728}
            argv = build_spa_emit_argv(plan, runtime)
            joined = " ".join(argv)
            self.assertIn("--pix_dir=", joined)
            self.assertIn("--borders_dir=", joined)
            self.assertIn("--iso=", joined)
            self.assertIn("--mode=production", joined)

    def test_rings_argv_uses_extract_script_and_script_exists(self):
        self.assertTrue(os.path.isfile(EXTRACT_RINGS_SCRIPT))
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="file:///tmp/finland.osm.pbf",
                out=os.path.join(tmp, "out"),
                borders_dir=borders,
                dry_run=True,
            )
            argv = build_rings_argv(plan, "/tmp/finland.osm.pbf")
            self.assertEqual(EXTRACT_RINGS_SCRIPT, argv[1])
            self.assertIn("--pbf", argv)
            self.assertIn("--out-jsonl", argv)

    def test_mapgen_skip_joined_as_skip_flag(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="file:///tmp/finland.osm.pbf",
                out=os.path.join(tmp, "out"),
                borders_dir=borders,
                dry_run=True,
            )
            argv = build_mapgen_argv(plan)
            self.assertIn("--skip", argv)
            skip_value = argv[argv.index("--skip") + 1]
            for token in plan["mapgen_skip"]:
                self.assertIn(token, skip_value.split(","))


class PlanetMd5Test(unittest.TestCase):
    def test_dry_run_does_not_write_md5_sidecar(self):
        with tempfile.TemporaryDirectory() as tmp:
            pbf = os.path.join(tmp, "extract.osm.pbf")
            with open(pbf, "wb") as f:
                f.write(b"pbf-bytes")
            borders = _finland_borders(tmp)
            with mock.patch(
                "street_pixels.map_pipeline.write_md5_for_file"
            ) as write_md5:
                run_map_pipeline(
                    pbf="file://{}".format(pbf),
                    out=os.path.join(tmp, "out"),
                    borders_dir=borders,
                    dry_run=True,
                )
            write_md5.assert_not_called()
            self.assertFalse(os.path.isfile(pbf + ".md5"))

    def test_predicted_missing_file_url_still_writes_on_real_ensure(self):
        with tempfile.TemporaryDirectory() as tmp:
            pbf = os.path.join(tmp, "extract.osm.pbf")
            with open(pbf, "wb") as f:
                f.write(b"abc")
            predicted = "file://{}".format(pbf + ".md5")
            url = ensure_planet_md5_url(
                "file://{}".format(pbf), predicted, dry_run=False
            )
            self.assertTrue(os.path.isfile(pbf + ".md5"))
            self.assertTrue(url.endswith(".md5"))
            with open(pbf + ".md5", "r", encoding="utf-8") as f:
                body = f.read()
            self.assertTrue(body.split()[0])

    def test_run_mapgen_writes_sidecar_when_plan_predicted_missing_md5(self):
        with tempfile.TemporaryDirectory() as tmp:
            pbf = os.path.join(tmp, "extract.osm.pbf")
            with open(pbf, "wb") as f:
                f.write(b"abc")
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="file://{}".format(pbf),
                out=os.path.join(tmp, "out"),
                borders_dir=borders,
                work_dir=os.path.join(tmp, "work"),
                dry_run=True,
            )
            self.assertIsNone(plan["planet_md5_explicit"])
            self.assertFalse(os.path.isfile(pbf + ".md5"))
            mwm_dir = os.path.join(tmp, "mwm")
            os.makedirs(mwm_dir)
            with open(os.path.join(mwm_dir, "countries.txt"), "w", encoding="utf-8") as f:
                f.write('{"v": 260728}\n')
            with mock.patch(
                "street_pixels.map_pipeline.run_command"
            ) as run_command, mock.patch(
                "street_pixels.map_pipeline.discover_mwm_dir", return_value=mwm_dir
            ), mock.patch(
                "street_pixels.map_pipeline.discover_planet_pbf", return_value=pbf
            ):
                run_mapgen(plan)
            run_command.assert_called_once()
            self.assertTrue(os.path.isfile(pbf + ".md5"))


class ThreadsCapTest(unittest.TestCase):
    def test_threads_zero_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            with self.assertRaises(MapPipelineError) as ctx:
                build_plan(
                    pbf="file:///tmp/finland.osm.pbf",
                    out=os.path.join(tmp, "out"),
                    borders_dir=borders,
                    threads=0,
                    dry_run=True,
                )
            self.assertIn("THREADS_COUNT", str(ctx.exception))


class ExtrasSkipTest(unittest.TestCase):
    def test_default_extras_skip_with_warning(self):
        with tempfile.TemporaryDirectory() as tmp:
            borders = _finland_borders(tmp)
            plan = build_plan(
                pbf="file:///tmp/finland.osm.pbf",
                out=os.path.join(tmp, "out"),
                borders_dir=borders,
                dry_run=True,
            )
            self.assertFalse(plan["mapgen_production"])
            expected_skip = [
                "Ugc",
                "RoutingTransit",
                "Srtm",
                "IsolinesInfo",
                "DownloadDescriptions",
                "Descriptions",
                "Popularity",
                "PopularityWorld",
                "Reviews",
            ]
            self.assertEqual(expected_skip, plan["mapgen_skip"])
            for stage in expected_skip:
                self.assertIn(stage, MAPGEN_SKIP_STAGE_NAMES)
            joined = " ".join(plan["extra_warnings"]).lower()
            self.assertIn("hotels", joined)
            self.assertIn("subway", joined)
            self.assertIn("srtm", joined)


if __name__ == "__main__":
    unittest.main()
