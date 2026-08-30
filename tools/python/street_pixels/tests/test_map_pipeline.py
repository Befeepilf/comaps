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
from street_pixels.map_pipeline import MapPipelineError  # noqa: E402
from street_pixels.map_pipeline import STAGE_ASSEMBLE  # noqa: E402
from street_pixels.map_pipeline import STAGE_MAPGEN  # noqa: E402
from street_pixels.map_pipeline import STAGE_PIX_DERIVE  # noqa: E402
from street_pixels.map_pipeline import STAGE_RINGS  # noqa: E402
from street_pixels.map_pipeline import STAGE_RSYNC  # noqa: E402
from street_pixels.map_pipeline import STAGE_SPA_EMIT  # noqa: E402
from street_pixels.map_pipeline import build_plan  # noqa: E402
from street_pixels.map_pipeline import expand_countries  # noqa: E402
from street_pixels.map_pipeline import load_default_ini_text  # noqa: E402
from street_pixels.map_pipeline import pipeline_stage_names  # noqa: E402
from street_pixels.map_pipeline import run_map_pipeline  # noqa: E402


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
            for stage in ("Srtm", "IsolinesInfo", "Ugc", "DownloadDescriptions"):
                self.assertIn(stage, plan["mapgen_skip"])
            joined = " ".join(plan["extra_warnings"]).lower()
            self.assertIn("hotels", joined)
            self.assertIn("subway", joined)
            self.assertIn("srtm", joined)


if __name__ == "__main__":
    unittest.main()
