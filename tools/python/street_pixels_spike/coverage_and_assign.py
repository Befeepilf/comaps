#!/usr/bin/env python3
"""Settlement coverage, PIP assignment timing, and assignment-table size estimates (SP-023)."""

from __future__ import annotations

import argparse
import json
import sys
import time
from collections import defaultdict
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Set, Tuple

import osmium
from shapely.geometry import LineString, MultiPolygon, Point, Polygon
from shapely.strtree import STRtree

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sp023_common import (  # noqa: E402
  SETTLEMENT_PLACES,
  densify_line,
  haversine_m,
  iter_jsonl,
  load_poly_file,
  ring_record_to_geometry,
  write_json,
)

try:
  import healpy as hp
except ImportError:  # pragma: no cover
  hp = None


NSIDE = 1048576  # SPD-017
STEP_M = 15.0
TARGET_N = 6_500_000  # Uusimaa-class Phase 3 reference


def geographic_area_m2(geom) -> float:
  """Rough geodesic area via equal-area cylindrical (sufficient for relative ranking)."""
  # Use shapely area on lon/lat scaled by cos(lat) — good enough for smallest-area tie-break
  # within a single country at Finland latitudes.
  if geom is None or geom.is_empty:
    return 0.0
  c = geom.centroid
  # meters per degree at centroid
  m_per_deg_lat = 111_320.0
  m_per_deg_lon = 111_320.0 * max(0.2, abs(__import__("math").cos(__import__("math").radians(c.y))))
  return float(geom.area) * m_per_deg_lat * m_per_deg_lon


class SettlementAreaHandler(osmium.SimpleHandler):
  """Closed settlement polygons: place=city|town|village|municipality OR admin_level=8."""

  def __init__(self) -> None:
    super().__init__()
    from osmium.geom import WKTFactory
    from shapely import wkt as shapely_wkt

    self.wkt = WKTFactory()
    self.shapely_wkt = shapely_wkt
    self.records: List[Dict[str, Any]] = []

  def area(self, a: osmium.osm.Area) -> None:
    tags = a.tags
    place = tags.get("place", "")
    admin = tags.get("admin_level", "")
    boundary = tags.get("boundary", "")
    keep = False
    class_key = ""
    if place in SETTLEMENT_PLACES:
      keep = True
      class_key = f"place_{place}"
    elif boundary == "administrative" and admin == "8":
      keep = True
      class_key = "admin_8"
    if not keep:
      return
    try:
      wkt_s = self.wkt.create_multipolygon(a)
      geom = self.shapely_wkt.loads(wkt_s)
    except Exception:
      return
    if geom.is_empty:
      return
    if not geom.is_valid:
      geom = geom.buffer(0)
    if geom.is_empty:
      return
    try:
      osm_id = int(a.orig_id())
    except Exception:
      osm_id = int(a.id)
    name = tags.get("name:en") or tags.get("name") or tags.get("name:fi") or ""
    self.records.append(
      {
        "osm_type": "way" if a.from_way() else "relation",
        "osm_id": osm_id,
        "name": name,
        "class_key": class_key,
        "place": place,
        "admin_level": int(admin) if admin.isdigit() else None,
        "geom": geom,
        "area_m2": geographic_area_m2(geom),
      }
    )


class HighwaySampleHandler(osmium.SimpleHandler):
  """Collect highway centerlines inside a bbox for HEALPix proxy sampling."""

  HIGHWAY_OK = frozenset(
    {
      "residential",
      "living_street",
      "pedestrian",
      "footway",
      "path",
      "cycleway",
      "tertiary",
      "secondary",
      "primary",
      "unclassified",
      "service",
      "track",
      "trunk",
      "road",
    }
  )

  def __init__(self, bbox: Tuple[float, float, float, float]) -> None:
    super().__init__()
    self.bbox = bbox
    self.lines: List[List[Tuple[float, float]]] = []

  def way(self, w: osmium.osm.Way) -> None:
    hw = w.tags.get("highway")
    if hw not in self.HIGHWAY_OK:
      return
    if not w.is_closed() and len(w.nodes) < 2:
      return
    coords: List[Tuple[float, float]] = []
    minx, miny, maxx, maxy = self.bbox
    any_in = False
    for n in w.nodes:
      if not n.location.valid():
        return
      lon, lat = n.location.lon, n.location.lat
      if minx <= lon <= maxx and miny <= lat <= maxy:
        any_in = True
      coords.append((lon, lat))
    if any_in and len(coords) >= 2:
      self.lines.append(coords)


def load_rings_as_areas(path: Path, class_keys: Optional[Set[str]] = None) -> List[Dict[str, Any]]:
  out = []
  for rec in iter_jsonl(path):
    if class_keys is not None and rec["class_key"] not in class_keys:
      continue
    geom = ring_record_to_geometry(rec)
    if geom is None or geom.is_empty:
      continue
    out.append(
      {
        "osm_type": rec["osm_type"],
        "osm_id": rec["osm_id"],
        "name": rec.get("name") or "",
        "class_key": rec["class_key"],
        "admin_level": rec.get("admin_level"),
        "place": rec.get("place") or "",
        "geom": geom,
        "area_m2": geographic_area_m2(geom),
      }
    )
  return out


def build_tree(areas: List[Dict[str, Any]]) -> Tuple[STRtree, List[Dict[str, Any]]]:
  geoms = [a["geom"] for a in areas]
  return STRtree(geoms), areas


def assign_point(pt: Point, tree: STRtree, areas: List[Dict[str, Any]]) -> Optional[Dict[str, Any]]:
  """Smallest containing polygon; OSM id ascending tie-break (spec §8.8)."""
  idxs = tree.query(pt)
  candidates = []
  for i in idxs:
    # shapely 2 returns ndarray of indices
    idx = int(i)
    a = areas[idx]
    try:
      if a["geom"].covers(pt):
        candidates.append(a)
    except Exception:
      continue
  if not candidates:
    return None
  candidates.sort(key=lambda a: (a["area_m2"], a["osm_id"]))
  return candidates[0]


def healpix_unique(points: Sequence[Tuple[float, float]]) -> Set[int]:
  if hp is None:
    raise RuntimeError("healpy is required for highway_healpix_proxy mode")
  # lon/lat → theta/phi (healpy uses colatitude theta, lon phi)
  import numpy as np

  lons = np.array([p[0] for p in points], dtype=float)
  lats = np.array([p[1] for p in points], dtype=float)
  theta = np.radians(90.0 - lats)
  phi = np.radians(lons)
  pix = hp.ang2pix(NSIDE, theta, phi, nest=True)
  return set(int(x) for x in pix)


def coverage_stats(
  settlements: List[Dict[str, Any]], subdivisions: List[Dict[str, Any]]
) -> Dict[str, Any]:
  if not settlements:
    return {"settlements": 0, "with_subdivision": 0, "pct": 0.0}
  sub_tree, sub_areas = build_tree(subdivisions)
  with_sub = 0
  examples = []
  for s in settlements:
    # Representative point: centroid must be inside settlement.
    pt = s["geom"].representative_point()
    idxs = sub_tree.query(s["geom"])
    found = False
    for i in idxs:
      sub = sub_areas[int(i)]
      try:
        if s["geom"].intersects(sub["geom"]) and not s["geom"].touches(sub["geom"]):
          # subdivision meaningfully overlaps settlement
          inter = s["geom"].intersection(sub["geom"])
          if not inter.is_empty and geographic_area_m2(inter) > 1000:
            found = True
            break
      except Exception:
        continue
    if found:
      with_sub += 1
      if len(examples) < 8:
        examples.append({"settlement": s["name"], "osm_id": s["osm_id"], "class": s["class_key"]})
  return {
    "settlements": len(settlements),
    "with_subdivision": with_sub,
    "pct": 100.0 * with_sub / len(settlements),
    "examples_with_subdivision": examples,
  }


def validate_helsinki_sample(subdivisions: List[Dict[str, Any]], helsinki_poly: MultiPolygon) -> Dict[str, Any]:
  """Programmatic spot-check: closed rings, names, nesting, known relation IDs."""
  inside = []
  for a in subdivisions:
    c = a["geom"].centroid
    if helsinki_poly.contains(c) or helsinki_poly.intersects(a["geom"]):
      inside.append(a)

  closed_ok = 0
  named = 0
  for a in inside:
    g = a["geom"]
    ok = True
    polys = [g] if isinstance(g, Polygon) else list(g.geoms)
    for p in polys:
      coords = list(p.exterior.coords)
      if len(coords) < 4 or coords[0] != coords[-1]:
        ok = False
    if ok:
      closed_ok += 1
    if a["name"]:
      named += 1

  # Known Helsinki district / neighbourhood relation IDs (OSM, stable-ish).
  # These are real admin/place polygons used for spot-check — not an allowlist.
  known_ids = {
    1236473: "Kamppi",  # often neighbourhood/district — may vary
    1723297: "Kallio",
    1723301: "Punavuori",
    1497225: "Töölö",
    1723318: "Ullanlinna",
    1833519: "Helsinki",  # municipality admin_8 often
  }
  found_known = []
  id_set = {a["osm_id"] for a in inside}
  # Also search all subdivisions for known ids
  all_ids = {a["osm_id"]: a for a in subdivisions}
  for kid, label in known_ids.items():
    if kid in all_ids:
      found_known.append(
        {
          "osm_id": kid,
          "expected_name_hint": label,
          "actual_name": all_ids[kid]["name"],
          "class_key": all_ids[kid]["class_key"],
          "closed": True,
          "area_m2": all_ids[kid]["area_m2"],
        }
      )

  # Nesting sanity: smaller place polygons should mostly lie within admin_8 Helsinki if present
  helsinki_admin = [a for a in subdivisions if a["name"] in ("Helsinki", "Helsingin kaupunki") and a.get("admin_level") == 8]
  nesting = {"helsinki_admin8_count": len(helsinki_admin), "place_inside_admin8_pct": None}
  if helsinki_admin:
    h = helsinki_admin[0]["geom"]
    places = [a for a in inside if a["class_key"].startswith("place_")]
    if places:
      inside_n = 0
      for p in places:
        try:
          if h.covers(p["geom"].representative_point()):
            inside_n += 1
        except Exception:
          pass
      nesting["place_inside_admin8_pct"] = 100.0 * inside_n / len(places)
      nesting["place_count"] = len(places)

  return {
    "polygons_intersecting_helsinki_mwm_border": len(inside),
    "closed_rings_ok": closed_ok,
    "named_count": named,
    "known_osm_ids_found": found_known,
    "nesting": nesting,
    "sample_names": sorted({a["name"] for a in inside if a["name"]})[:40],
  }


def assignment_table_estimates(n_pixels: int, n_areas: int) -> Dict[str, Any]:
  """Rough on-disk estimates for N≈6.5e6 valid street pixels."""
  # Area id as uint32 OSM-local index, or uint64 osm id, or varint index.
  return {
    "N_pixels_reference": n_pixels,
    "M_candidate_areas": n_areas,
    "full_universe_uint32_area_index_bytes": n_pixels * 4,
    "full_universe_uint64_osm_id_bytes": n_pixels * 8,
    "full_universe_uint16_if_M_fits_bytes": n_pixels * 2 if n_areas < 65535 else None,
    "sparse_explored_only_note": "size ≈ explored_count × (8 byte healpix + area id); explored << N for most users",
    "sparse_10pct_explored_uint32_bytes": int(0.10 * n_pixels) * (8 + 4),
    "sparse_1pct_explored_uint32_bytes": int(0.01 * n_pixels) * (8 + 4),
    "rematerialize_on_demand_bytes": 0,
    "rematerialize_note": "no persistent table; pay PIP (or precomputed blob download) at rematch/derive",
  }


def main() -> int:
  ap = argparse.ArgumentParser(description=__doc__)
  ap.add_argument("--rings", required=True, type=Path)
  ap.add_argument("--pbf", required=True, type=Path)
  ap.add_argument("--helsinki-poly", required=True, type=Path)
  ap.add_argument("--out", required=True, type=Path)
  ap.add_argument(
    "--universe-mode",
    choices=("highway_healpix_proxy",),
    default="highway_healpix_proxy",
    help="No .pix in workspace; default is highway→HEALPix nside=1048576 proxy at 15 m",
  )
  ap.add_argument("--pip-sample", type=int, default=50000, help="Max unique pixels for timed PIP")
  ap.add_argument("--max-highway-ways", type=int, default=0, help="0 = all in bbox")
  args = ap.parse_args()

  print("loading Helsinki border …", flush=True)
  helsinki_poly = load_poly_file(args.helsinki_poly)
  minx, miny, maxx, maxy = helsinki_poly.bounds
  bbox = (minx, miny, maxx, maxy)
  print(f"  bbox={bbox}", flush=True)

  # Subdivisions for coverage / assignment: admin 9–11 + place suburb/quarter/neighbourhood
  # Also keep admin 8 for settlement fallback geometry from rings file.
  print("loading retained rings …", flush=True)
  all_areas = load_rings_as_areas(args.rings)
  subdivisions = [
    a
    for a in all_areas
    if a["class_key"] in ("admin_9", "admin_10", "admin_11", "place_suburb", "place_quarter", "place_neighbourhood")
  ]
  admin8 = [a for a in all_areas if a["class_key"] == "admin_8"]
  print(f"  subdivisions={len(subdivisions)} admin8={len(admin8)}", flush=True)

  # Settlements: prefer polygonal place city/town/village + admin_8 from PBF (may overlap rings).
  print("extracting settlement polygons from PBF (second pass) …", flush=True)
  sh = SettlementAreaHandler()
  sh.apply_file(str(args.pbf), locations=True, idx="flex_mem")
  settlements = sh.records
  # Restrict coverage denominator to settlements overlapping Helsinki MWM for Uusimaa-class focus,
  # and also report national.
  settlements_hel = []
  for s in settlements:
    try:
      if helsinki_poly.intersects(s["geom"]):
        settlements_hel.append(s)
    except Exception:
      continue
  print(f"  settlements_national={len(settlements)} settlements_helsinki_mwm={len(settlements_hel)}", flush=True)

  cov_national = coverage_stats(settlements, subdivisions)
  cov_hel = coverage_stats(settlements_hel, subdivisions)

  validation = validate_helsinki_sample(subdivisions + admin8, helsinki_poly)

  # Highway → HEALPix proxy universe inside Helsinki border
  print("sampling highways for HEALPix proxy …", flush=True)
  hh = HighwaySampleHandler(bbox)
  hh.apply_file(str(args.pbf), locations=True)
  lines = hh.lines
  if args.max_highway_ways and len(lines) > args.max_highway_ways:
    lines = lines[: args.max_highway_ways]
  print(f"  highway ways in bbox: {len(lines)}", flush=True)

  points: List[Tuple[float, float]] = []
  for coords in lines:
    densified = densify_line(coords, STEP_M)
    for lon, lat in densified:
      if helsinki_poly.contains(Point(lon, lat)):
        points.append((lon, lat))

  print(f"  densified points inside border: {len(points)}", flush=True)
  t0 = time.perf_counter()
  pix_set = healpix_unique(points) if points else set()
  t_heal = time.perf_counter() - t0
  n_proxy = len(pix_set)
  print(f"  unique HEALPix cells (nside={NSIDE}): {n_proxy} in {t_heal:.2f}s", flush=True)

  # Build assignment candidate set: subdivisions first; settlement fallback = admin8 + place settlements
  assign_candidates = list(subdivisions)
  # For PIP cost, also time against subdivisions-only (exploration areas) then fallback
  tree, areas = build_tree(assign_candidates)

  # Sample points for PIP: take up to pip_sample unique pixels' representative points
  # Re-derive lon/lat from a subset of densified points mapped to unique pix (approx).
  sample_pts: List[Tuple[float, float]] = []
  seen_pix: Set[int] = set()
  if hp is not None and points:
    import numpy as np

    for lon, lat in points:
      theta = __import__("math").radians(90.0 - lat)
      phi = __import__("math").radians(lon)
      pix = int(hp.ang2pix(NSIDE, theta, phi, nest=True))
      if pix in seen_pix:
        continue
      seen_pix.add(pix)
      sample_pts.append((lon, lat))
      if len(sample_pts) >= args.pip_sample:
        break

  print(f"PIP timing on {len(sample_pts)} unique proxy pixels …", flush=True)
  assigned_sub = 0
  assigned_fallback = 0
  none_area = 0
  # Fallback tree: settlements (place+admin8) intersecting Helsinki
  fallback_areas = [s for s in settlements_hel]
  fb_tree, fb_areas = build_tree(fallback_areas) if fallback_areas else (None, [])

  t0 = time.perf_counter()
  for lon, lat in sample_pts:
    pt = Point(lon, lat)
    hit = assign_point(pt, tree, areas) if areas else None
    if hit is not None:
      assigned_sub += 1
      continue
    if fb_tree is not None:
      fb = assign_point(pt, fb_tree, fb_areas)
      if fb is not None:
        assigned_fallback += 1
        continue
    none_area += 1
  t_pip = time.perf_counter() - t0
  n_sample = max(1, len(sample_pts))
  per_pt_us = (t_pip / n_sample) * 1e6
  # Extrapolate to full proxy and to N=6.5e6
  extrapolate = {
    "sample_n": n_sample,
    "sample_seconds": t_pip,
    "per_point_us": per_pt_us,
    "proxy_universe_n": n_proxy,
    "est_seconds_for_proxy_universe": per_pt_us * n_proxy / 1e6,
    "est_seconds_for_N_6_5e6": per_pt_us * TARGET_N / 1e6,
    "est_minutes_for_N_6_5e6": per_pt_us * TARGET_N / 1e6 / 60.0,
    "hardware_note": "desktop x86_64 cloud VM (not phone); treat as optimistic lower bound for on-device",
  }

  pixel_buckets = {
    "in_subdivision": assigned_sub,
    "settlement_fallback_only": assigned_fallback,
    "no_area": none_area,
    "pct_subdivision": 100.0 * assigned_sub / n_sample,
    "pct_fallback": 100.0 * assigned_fallback / n_sample,
    "pct_no_area": 100.0 * none_area / n_sample,
  }

  table_est = assignment_table_estimates(TARGET_N, len(assign_candidates))
  table_est_proxy = assignment_table_estimates(max(n_proxy, 1), len(assign_candidates))

  report = {
    "universe": {
      "mode": args.universe_mode,
      "nside": NSIDE,
      "step_m": STEP_M,
      "note": "No .pix in workspace; highway densify @15m → HEALPix nest unique cells inside Helsinki MWM border",
      "highway_ways": len(hh.lines),
      "densified_points_inside_border": len(points),
      "unique_healpix_cells": n_proxy,
      "phase3_reference_N": TARGET_N,
      "proxy_vs_reference_ratio": n_proxy / TARGET_N if TARGET_N else None,
    },
    "coverage": {
      "national_settlements": cov_national,
      "helsinki_mwm_settlements": cov_hel,
      "subdivision_classes": sorted({a["class_key"] for a in subdivisions}),
      "subdivision_count": len(subdivisions),
    },
    "pixel_assignment_sample": pixel_buckets,
    "pip_cost": extrapolate,
    "assignment_table_estimates_N_6_5e6": table_est,
    "assignment_table_estimates_proxy_N": table_est_proxy,
    "manual_validation": validation,
    "recommendation_inputs_seed": {
      "store": "compare finland zlib_coded (size_report) vs World cities_boundaries (~1MB) and packed_polygons (~3.6MB) and Helsinki MWM (~125MB)",
      "assignment_locus": "use est_seconds_for_N_6_5e6 vs rematch budget; desktop timing is optimistic",
      "persistence": "full uint32 map at 6.5e6 = 26 MB; sparse explored-only much smaller; rematerialize = 0 store",
    },
  }
  write_json(args.out, report)
  print(json.dumps({"coverage_hel": cov_hel, "pip": extrapolate, "pixels": pixel_buckets}, indent=2))
  print(f"wrote {args.out}", flush=True)
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
