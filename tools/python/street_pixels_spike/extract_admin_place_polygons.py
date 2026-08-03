#!/usr/bin/env python3
"""Extract closed admin_level 5–11 and place suburb|quarter|neighbourhood rings from a PBF.

Never synthesises polygons around place nodes. Desktop spike only (SP-023).
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import osmium
from osmium.geom import WKTFactory
from shapely import wkt as shapely_wkt
from shapely.geometry import MultiPolygon, Polygon, mapping

# Allow running as a script without installing the package.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from sp023_common import ADMIN_LEVELS, PLACE_TYPES, write_json  # noqa: E402


def _name_from_tags(tags) -> str:
  for key in ("name:en", "name", "name:fi", "official_name", "alt_name"):
    if key in tags:
      return tags[key]
  return ""


def _classify(tags) -> Optional[Dict[str, Any]]:
  boundary = tags.get("boundary")
  admin_level = tags.get("admin_level")
  place = tags.get("place")

  if boundary == "administrative" and admin_level:
    try:
      level = int(admin_level)
    except ValueError:
      return None
    if level in ADMIN_LEVELS:
      return {
        "kind": "admin",
        "admin_level": level,
        "place": place or "",
        "class_key": f"admin_{level}",
      }

  if place in PLACE_TYPES:
    # Closed polygonal place only — AreaHandler already requires area geometry.
    return {
      "kind": "place",
      "admin_level": int(admin_level) if admin_level and admin_level.isdigit() else None,
      "place": place,
      "class_key": f"place_{place}",
    }
  return None


def _rings_from_geom(geom) -> List[List[List[float]]]:
  rings: List[List[List[float]]] = []

  def add_poly(poly: Polygon) -> None:
    # Outer only (matches PlaceBoundariesHolder: store outers; holes ignored).
    coords = [[float(x), float(y)] for x, y in poly.exterior.coords]
    if len(coords) >= 4:
      rings.append(coords)

  if isinstance(geom, Polygon):
    add_poly(geom)
  elif isinstance(geom, MultiPolygon):
    for g in geom.geoms:
      add_poly(g)
  return rings


class ExtractHandler(osmium.SimpleHandler):
  def __init__(self) -> None:
    super().__init__()
    self.wkt = WKTFactory()
    self.records: List[Dict[str, Any]] = []
    self.stats = {
      "areas_seen": 0,
      "areas_kept": 0,
      "areas_invalid_geom": 0,
      "areas_open_or_empty": 0,
      "by_class": {},
    }

  def area(self, a: osmium.osm.Area) -> None:
    self.stats["areas_seen"] += 1
    tags = a.tags
    cls = _classify(tags)
    if cls is None:
      return
    try:
      wkt_s = self.wkt.create_multipolygon(a)
    except Exception:
      self.stats["areas_invalid_geom"] += 1
      return
    if not wkt_s:
      self.stats["areas_invalid_geom"] += 1
      return
    try:
      geom = shapely_wkt.loads(wkt_s)
    except Exception:
      self.stats["areas_invalid_geom"] += 1
      return
    if geom.is_empty:
      self.stats["areas_open_or_empty"] += 1
      return
    if not geom.is_valid:
      geom = geom.buffer(0)
    if geom.is_empty:
      self.stats["areas_open_or_empty"] += 1
      return

    rings = _rings_from_geom(geom)
    if not rings:
      self.stats["areas_open_or_empty"] += 1
      return

    # Require closed rings (first==last) — shapely Polygon already closes.
    for ring in rings:
      if ring[0] != ring[-1] or len(ring) < 4:
        self.stats["areas_open_or_empty"] += 1
        return

    osm_type = "relation" if a.from_way() is False else "way"
    # a.orig_id() / a.id: Area id encoding — use original object id.
    try:
      osm_id = int(a.orig_id())
    except Exception:
      osm_id = int(a.id)

    area_m2 = float(geom.area)  # deg² — used only for relative smallest-area
    # Prefer geographic area estimate via equal-area projection later; store both.
    rec = {
      "osm_type": osm_type,
      "osm_id": osm_id,
      "name": _name_from_tags(tags),
      "kind": cls["kind"],
      "admin_level": cls["admin_level"],
      "place": cls["place"],
      "class_key": cls["class_key"],
      "vertex_count": sum(len(r) for r in rings),
      "ring_count": len(rings),
      "area_deg2": area_m2,
      "bbox": list(geom.bounds),  # minx,miny,maxx,maxy
      "rings": rings,
      "centroid": [float(geom.centroid.x), float(geom.centroid.y)],
    }
    self.records.append(rec)
    self.stats["areas_kept"] += 1
    self.stats["by_class"][cls["class_key"]] = self.stats["by_class"].get(cls["class_key"], 0) + 1


def _in_bbox(rec: Dict[str, Any], bbox: Tuple[float, float, float, float]) -> bool:
  minx, miny, maxx, maxy = rec["bbox"]
  bx0, by0, bx1, by1 = bbox
  return not (maxx < bx0 or minx > bx1 or maxy < by0 or miny > by1)


def export_helsinki_geojson(records: List[Dict[str, Any]], out_path: Path, bbox: Tuple[float, float, float, float]) -> int:
  features = []
  for rec in records:
    if rec["class_key"] not in {
      "admin_9",
      "admin_10",
      "admin_11",
      "place_suburb",
      "place_quarter",
      "place_neighbourhood",
    } and rec.get("admin_level") not in (8, 9, 10, 11):
      # Keep helsinki-relevant subdivisions + municipalities for nesting checks
      if rec["class_key"] not in ("admin_8", "admin_9", "admin_10", "admin_11") and not rec[
        "class_key"
      ].startswith("place_"):
        continue
    if not _in_bbox(rec, bbox):
      continue
    rings = rec["rings"]
    if len(rings) == 1:
      geom = {"type": "Polygon", "coordinates": rings}
    else:
      geom = {"type": "MultiPolygon", "coordinates": [[r] for r in rings]}
    features.append(
      {
        "type": "Feature",
        "properties": {
          "osm_type": rec["osm_type"],
          "osm_id": rec["osm_id"],
          "name": rec["name"],
          "kind": rec["kind"],
          "admin_level": rec["admin_level"],
          "place": rec["place"],
          "class_key": rec["class_key"],
          "vertex_count": rec["vertex_count"],
        },
        "geometry": geom,
      }
    )
  fc = {"type": "FeatureCollection", "features": features}
  write_json(out_path, fc)
  return len(features)


def main() -> int:
  ap = argparse.ArgumentParser(description=__doc__)
  ap.add_argument("--pbf", required=True, type=Path)
  ap.add_argument("--out-jsonl", required=True, type=Path)
  ap.add_argument("--out-geojson-helsinki", type=Path, default=None)
  ap.add_argument("--out-stats", type=Path, default=None)
  args = ap.parse_args()

  print(f"extracting from {args.pbf} …", flush=True)
  h = ExtractHandler()
  # locations=True required for area assembly; idx keeps node locations.
  h.apply_file(str(args.pbf), locations=True, idx="flex_mem")
  print(f"kept {h.stats['areas_kept']} / seen {h.stats['areas_seen']}", flush=True)
  print(f"by_class: {json.dumps(h.stats['by_class'], sort_keys=True)}", flush=True)

  args.out_jsonl.parent.mkdir(parents=True, exist_ok=True)
  with args.out_jsonl.open("w", encoding="utf-8") as f:
    for rec in h.records:
      f.write(json.dumps(rec, ensure_ascii=False) + "\n")

  stats_path = args.out_stats or args.out_jsonl.with_suffix(".stats.json")
  write_json(stats_path, h.stats)

  if args.out_geojson_helsinki:
    # Approximate Helsinki metro bbox (covers Uusimaa Helsinki MWM core).
    bbox = (24.5, 59.9, 25.6, 60.5)
    n = export_helsinki_geojson(h.records, args.out_geojson_helsinki, bbox)
    print(f"helsinki geojson features: {n} → {args.out_geojson_helsinki}", flush=True)

  return 0


if __name__ == "__main__":
  raise SystemExit(main())
