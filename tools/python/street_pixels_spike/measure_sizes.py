#!/usr/bin/env python3
"""Measure raw / coded-delta / zlib sizes for retained admin/place rings (SP-023)."""

from __future__ import annotations

import argparse
import json
import sys
import zlib
from collections import defaultdict
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

from shapely.geometry import Point

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sp023_common import (  # noqa: E402
  coded_delta_bytes,
  iter_jsonl,
  load_finland_borders,
  raw_pod_bytes,
  read_mwm_sections,
  ring_record_to_geometry,
  write_json,
)


def measure_record(rec: Dict[str, Any]) -> Dict[str, int]:
  rings = rec["rings"]
  raw = raw_pod_bytes(rings)
  coded = coded_delta_bytes(rings)
  return {
    "vertices": int(rec.get("vertex_count") or sum(len(r) for r in rings)),
    "raw_pod": len(raw),
    "coded_delta": len(coded),
    "zlib_raw": len(zlib.compress(raw, 9)),
    "zlib_coded": len(zlib.compress(coded, 9)),
  }


def attribute_mwm(rec: Dict[str, Any], borders: Dict[str, Any]) -> List[str]:
  """Attribute polygon to MWM borders by centroid containment (multi-ok)."""
  c = rec.get("centroid")
  if not c:
    return []
  pt = Point(float(c[0]), float(c[1]))
  hits = []
  for name, geom in borders.items():
    try:
      if geom.contains(pt) or geom.touches(pt):
        hits.append(name)
    except Exception:
      continue
  return hits


def main() -> int:
  ap = argparse.ArgumentParser(description=__doc__)
  ap.add_argument("--rings", required=True, type=Path)
  ap.add_argument("--borders-dir", type=Path, default=Path("data/borders"))
  ap.add_argument("--world-mwm", type=Path, default=None)
  ap.add_argument("--helsinki-mwm", type=Path, default=None)
  ap.add_argument("--packed-polygons", type=Path, default=None)
  ap.add_argument("--out", required=True, type=Path)
  args = ap.parse_args()

  borders = {}
  if args.borders_dir.exists():
    print(f"loading borders from {args.borders_dir} …", flush=True)
    borders = load_finland_borders(args.borders_dir)
    print(f"  {len(borders)} Finland_*.poly", flush=True)

  by_class: Dict[str, Dict[str, float]] = defaultdict(
    lambda: {
      "count": 0,
      "vertices": 0,
      "raw_pod": 0,
      "coded_delta": 0,
      "zlib_raw": 0,
      "zlib_coded": 0,
    }
  )
  by_mwm: Dict[str, Dict[str, float]] = defaultdict(
    lambda: {
      "count": 0,
      "vertices": 0,
      "raw_pod": 0,
      "coded_delta": 0,
      "zlib_raw": 0,
      "zlib_coded": 0,
    }
  )
  totals = {
    "count": 0,
    "vertices": 0,
    "raw_pod": 0,
    "coded_delta": 0,
    "zlib_raw": 0,
    "zlib_coded": 0,
  }
  # Concatenate all coded payloads for one country-level zlib (sidecar-like).
  all_coded = bytearray()
  all_raw = bytearray()

  helsinki_key = "Finland_Southern Finland_Helsinki"

  for rec in iter_jsonl(args.rings):
    m = measure_record(rec)
    ck = rec["class_key"]
    for k, v in m.items():
      by_class[ck][k] += v
    by_class[ck]["count"] += 1
    for k, v in m.items():
      totals[k] += v
    totals["count"] += 1
    all_coded.extend(coded_delta_bytes(rec["rings"]))
    all_raw.extend(raw_pod_bytes(rec["rings"]))

    if borders:
      hits = attribute_mwm(rec, borders)
      if not hits:
        by_mwm["(unattributed)"]["count"] += 1
        for k, v in m.items():
          by_mwm["(unattributed)"][k] += v
      for name in hits:
        by_mwm[name]["count"] += 1
        for k, v in m.items():
          by_mwm[name][k] += v

  country_zlib_coded = len(zlib.compress(bytes(all_coded), 9))
  country_zlib_raw = len(zlib.compress(bytes(all_raw), 9))

  baselines: Dict[str, Any] = {}
  if args.world_mwm and args.world_mwm.exists():
    sections = read_mwm_sections(args.world_mwm)
    baselines["World.mwm_bytes"] = args.world_mwm.stat().st_size
    baselines["World.cities_boundaries_bytes"] = sections.get("cities_boundaries")
    baselines["World.sections"] = {k: sections[k] for k in sorted(sections) if "cit" in k or "bound" in k}
  if args.helsinki_mwm and args.helsinki_mwm.exists():
    baselines["Finland_Southern_Finland_Helsinki.mwm_bytes"] = args.helsinki_mwm.stat().st_size
  if args.packed_polygons and args.packed_polygons.exists():
    baselines["packed_polygons.bin_bytes"] = args.packed_polygons.stat().st_size

  # Package delta estimates for Helsinki MWM if we had in-MWM coded polygons.
  helsinki_stats = by_mwm.get(helsinki_key, {})
  package_deltas = {
    "helsinki_mwm_plus_coded_delta_bytes": (
      baselines.get("Finland_Southern_Finland_Helsinki.mwm_bytes", 0) + int(helsinki_stats.get("coded_delta", 0))
      if helsinki_stats
      else None
    ),
    "helsinki_mwm_plus_zlib_coded_bytes": (
      baselines.get("Finland_Southern_Finland_Helsinki.mwm_bytes", 0) + int(helsinki_stats.get("zlib_coded", 0))
      if helsinki_stats
      else None
    ),
    "helsinki_coded_delta_vs_mwm_pct": (
      100.0 * helsinki_stats.get("coded_delta", 0) / baselines["Finland_Southern_Finland_Helsinki.mwm_bytes"]
      if helsinki_stats and baselines.get("Finland_Southern_Finland_Helsinki.mwm_bytes")
      else None
    ),
    "finland_zlib_coded_vs_packed_polygons": (
      country_zlib_coded / baselines["packed_polygons.bin_bytes"]
      if baselines.get("packed_polygons.bin_bytes")
      else None
    ),
    "finland_zlib_coded_vs_world_cities_boundaries": (
      country_zlib_coded / baselines["World.cities_boundaries_bytes"]
      if baselines.get("World.cities_boundaries_bytes")
      else None
    ),
  }

  report = {
    "totals": {k: int(v) if k != "count" else int(v) for k, v in totals.items()},
    "country_concat_zlib_raw": country_zlib_raw,
    "country_concat_zlib_coded": country_zlib_coded,
    "by_class": {k: {kk: int(vv) for kk, vv in v.items()} for k, v in sorted(by_class.items())},
    "by_mwm": {k: {kk: int(vv) for kk, vv in v.items()} for k, v in sorted(by_mwm.items())},
    "baselines": baselines,
    "package_deltas": package_deltas,
    "notes": [
      "raw_pod = float64 lon/lat per vertex + ring headers",
      "coded_delta = 1e7 scaled int delta + zigzag varint (spike approximation of geometry coding)",
      "zlib_* = zlib level 9 on that encoding; country_concat_* zlib over concatenation of all polygons",
      "PlaceBoundariesHolder keeps true rings then place_processor boxifies — this spike measures pre-boxify retention",
    ],
  }
  write_json(args.out, report)
  print(json.dumps({"totals": report["totals"], "country_concat_zlib_coded": country_zlib_coded}, indent=2))
  print(f"wrote {args.out}", flush=True)
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
