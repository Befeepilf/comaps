"""Shared helpers for Street Pixels desktop spikes (non-shipping)."""

from __future__ import annotations

import json
import math
import struct
from pathlib import Path
from typing import Any, Dict, Iterable, List, Sequence, Tuple

from shapely.geometry import MultiPolygon, Polygon


ADMIN_LEVELS = tuple(range(5, 12))
PLACE_TYPES = frozenset({"suburb", "quarter", "neighbourhood"})
SETTLEMENT_PLACES = frozenset({"city", "town", "village", "municipality"})


def load_poly_file(path: Path) -> MultiPolygon:
  """Parse an OSM `.poly` border file into a MultiPolygon (lon/lat)."""
  text = path.read_text(encoding="utf-8", errors="replace").splitlines()
  if not text:
    raise ValueError(f"Empty poly file: {path}")

  rings: List[List[Tuple[float, float]]] = []
  current: List[Tuple[float, float]] = []
  # Skip first line (name). Subsequent lines: ring header, coords, END, final END.
  for line in text[1:]:
    s = line.strip()
    if not s:
      continue
    if s == "END":
      if current:
        if current[0] != current[-1]:
          current.append(current[0])
        if len(current) >= 4:
          rings.append(current)
        current = []
      continue
    parts = s.split()
    if len(parts) >= 2:
      try:
        lon = float(parts[0])
        lat = float(parts[1])
      except ValueError:
        # Ring header with unexpected tokens — start fresh ring.
        if current:
          if current[0] != current[-1]:
            current.append(current[0])
          if len(current) >= 4:
            rings.append(current)
        current = []
        continue
      current.append((lon, lat))
    else:
      # Ring header (e.g. "1" or "!hole")
      if current:
        if current[0] != current[-1]:
          current.append(current[0])
        if len(current) >= 4:
          rings.append(current)
      current = []

  if current:
    if current[0] != current[-1]:
      current.append(current[0])
    if len(current) >= 4:
      rings.append(current)

  polys: List[Polygon] = []
  for ring in rings:
    try:
      p = Polygon(ring)
      if not p.is_valid:
        p = p.buffer(0)
      if p.is_empty:
        continue
      if isinstance(p, Polygon):
        polys.append(p)
      elif isinstance(p, MultiPolygon):
        polys.extend([g for g in p.geoms if isinstance(g, Polygon) and not g.is_empty])
    except Exception:
      continue
  if not polys:
    raise ValueError(f"No polygons parsed from {path}")
  return MultiPolygon(polys)


def load_finland_borders(borders_dir: Path) -> Dict[str, MultiPolygon]:
  out: Dict[str, MultiPolygon] = {}
  for path in sorted(borders_dir.glob("Finland_*.poly")):
    out[path.stem] = load_poly_file(path)
  return out


def ring_record_to_geometry(rec: Dict[str, Any]):
  coords = rec.get("rings") or []
  if not coords:
    return None
  polys = []
  for ring in coords:
    if len(ring) < 3:
      continue
    pts = [(float(p[0]), float(p[1])) for p in ring]
    if pts[0] != pts[-1]:
      pts.append(pts[0])
    try:
      poly = Polygon(pts)
      if not poly.is_valid:
        poly = poly.buffer(0)
      if poly.is_empty:
        continue
      if isinstance(poly, Polygon):
        polys.append(poly)
      elif isinstance(poly, MultiPolygon):
        polys.extend(list(poly.geoms))
    except Exception:
      continue
  if not polys:
    return None
  if len(polys) == 1:
    return polys[0]
  return MultiPolygon(polys)


def geom_vertex_count(geom) -> int:
  if geom is None or geom.is_empty:
    return 0
  if isinstance(geom, Polygon):
    n = len(geom.exterior.coords)
    for interior in geom.interiors:
      n += len(interior.coords)
    return n
  if isinstance(geom, MultiPolygon):
    return sum(geom_vertex_count(g) for g in geom.geoms)
  return 0


def iter_jsonl(path: Path) -> Iterable[Dict[str, Any]]:
  with path.open(encoding="utf-8") as f:
    for line in f:
      line = line.strip()
      if line:
        yield json.loads(line)


def write_json(path: Path, obj: Any) -> None:
  path.parent.mkdir(parents=True, exist_ok=True)
  with path.open("w", encoding="utf-8") as f:
    json.dump(obj, f, indent=2, ensure_ascii=False)
    f.write("\n")


def zigzag(n: int) -> int:
  return (n << 1) ^ (n >> 31)


def write_varint(buf: bytearray, value: int) -> None:
  v = value & 0xFFFFFFFFFFFFFFFF
  while True:
    byte = v & 0x7F
    v >>= 7
    if v:
      buf.append(byte | 0x80)
    else:
      buf.append(byte)
      break


def coded_delta_bytes(rings: Sequence[Sequence[Sequence[float]]], scale: float = 1e7) -> bytes:
  """Delta + zigzag-varint coding of lon/lat rings (approx production geometry coding)."""
  buf = bytearray()
  write_varint(buf, len(rings))
  for ring in rings:
    write_varint(buf, len(ring))
    prev_x = 0
    prev_y = 0
    for i, (lon, lat) in enumerate(ring):
      x = int(round(lon * scale))
      y = int(round(lat * scale))
      if i == 0:
        write_varint(buf, zigzag(x))
        write_varint(buf, zigzag(y))
      else:
        write_varint(buf, zigzag(x - prev_x))
        write_varint(buf, zigzag(y - prev_y))
      prev_x, prev_y = x, y
  return bytes(buf)


def raw_pod_bytes(rings: Sequence[Sequence[Sequence[float]]]) -> bytes:
  """Raw POD: float64 lon/lat pairs per vertex, plus ring lengths."""
  buf = bytearray()
  buf.extend(struct.pack("<I", len(rings)))
  for ring in rings:
    buf.extend(struct.pack("<I", len(ring)))
    for lon, lat in ring:
      buf.extend(struct.pack("<dd", float(lon), float(lat)))
  return bytes(buf)


def haversine_m(lon1: float, lat1: float, lon2: float, lat2: float) -> float:
  r = 6371000.0
  p1, p2 = math.radians(lat1), math.radians(lat2)
  dphi = math.radians(lat2 - lat1)
  dl = math.radians(lon2 - lon1)
  a = math.sin(dphi / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
  return 2 * r * math.asin(min(1.0, math.sqrt(a)))


def densify_line(coords: Sequence[Tuple[float, float]], step_m: float) -> List[Tuple[float, float]]:
  if len(coords) < 2:
    return list(coords)
  out: List[Tuple[float, float]] = []
  for i in range(1, len(coords)):
    lon0, lat0 = coords[i - 1]
    lon1, lat1 = coords[i]
    if i == 1:
      out.append((lon0, lat0))
    dist = haversine_m(lon0, lat0, lon1, lat1)
    if dist <= step_m:
      out.append((lon1, lat1))
      continue
    n = max(1, int(math.ceil(dist / step_m)))
    for k in range(1, n + 1):
      t = k / n
      out.append((lon0 + (lon1 - lon0) * t, lat0 + (lat1 - lat0) * t))
  return out


def read_mwm_sections(path: Path) -> Dict[str, int]:
  """Return {section_name: size_bytes} without requiring types.txt."""

  def read_varuint(mm: bytes, pos: int) -> Tuple[int, int]:
    res = 0
    shift = 0
    while True:
      b = mm[pos]
      pos += 1
      res |= (b & 0x7F) << shift
      if (b & 0x80) == 0:
        break
      shift += 7
    return res, pos

  data = path.read_bytes()
  offset = int.from_bytes(data[0:8], "little")
  pos = offset
  n, pos = read_varuint(data, pos)
  tags: Dict[str, int] = {}
  for _ in range(n):
    strlen, pos = read_varuint(data, pos)
    name = data[pos : pos + strlen].decode("utf-8")
    pos += strlen
    _off, pos = read_varuint(data, pos)
    length, pos = read_varuint(data, pos)
    tags[name] = length
  return tags
