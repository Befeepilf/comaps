#!/usr/bin/env python3
"""Optional offline helper: filter SP-023 JSONL rings by street_pixels policy.

Does not write .spa (C++ WriteExplorationSidecar owns the shipping codec).
Use to preview admit/reject counts before a local C++ emit harness.

Example:
  python3 tools/python/street_pixels_spike/filter_rings_for_spa.py \\
    --rings /tmp/sp023/finland_admin_place_rings.jsonl \\
    --policy data/street_pixels/country_policies.json \\
    --iso FI
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Dict, List, Tuple


def load_policy(path: Path, iso: str) -> Tuple[int, Dict[str, Any]]:
  root = json.loads(path.read_text(encoding="utf-8"))
  countries = root.get("countries") or {}
  if iso not in countries:
    raise SystemExit(f"ISO {iso} not in {path}")
  return int(root["policy_version"]), countries[iso]


def admit(rec: Dict[str, Any], policy: Dict[str, Any]) -> str | None:
  """Return reject reason, or None if admitted."""
  name = (rec.get("name") or "").strip()
  if not name:
    return "unnamed"
  rings = rec.get("rings") or []
  if not rings:
    return "empty_rings"
  for ring in rings:
    if len(ring) < 4 or ring[0] != ring[-1]:
      return "invalid_ring"

  kind = rec.get("kind")
  if kind == "admin":
    level = int(rec.get("admin_level", -1))
    if level in policy.get("subdivision_admin_levels", []):
      return None
    if level in policy.get("settlement_admin_levels", []):
      return None
    return "policy_mismatch"
  if kind == "place":
    place = policy.get("place_boundaries") or {}
    if not place.get("enabled"):
      return "policy_mismatch"
    if rec.get("place") not in (place.get("place_types") or []):
      return "policy_mismatch"
    return None
  return "policy_mismatch"


def main() -> int:
  ap = argparse.ArgumentParser(description=__doc__)
  ap.add_argument("--rings", type=Path, required=True)
  ap.add_argument("--policy", type=Path, required=True)
  ap.add_argument("--iso", default="FI")
  args = ap.parse_args()

  policy_version, policy = load_policy(args.policy, args.iso)
  counts: Dict[str, int] = {"admitted": 0}
  roles: Dict[str, int] = {}

  with args.rings.open(encoding="utf-8") as f:
    for line in f:
      line = line.strip()
      if not line:
        continue
      rec = json.loads(line)
      reason = admit(rec, policy)
      if reason:
        counts[reason] = counts.get(reason, 0) + 1
        continue
      counts["admitted"] += 1
      if rec.get("kind") == "admin":
        level = int(rec["admin_level"])
        if level in policy.get("settlement_admin_levels", []):
          roles["settlement"] = roles.get("settlement", 0) + 1
        else:
          roles["subdivision"] = roles.get("subdivision", 0) + 1
      else:
        roles["place"] = roles.get("place", 0) + 1

  print(json.dumps({"policy_version": policy_version, "iso": args.iso, "counts": counts, "roles": roles}, indent=2))
  return 0


if __name__ == "__main__":
  sys.exit(main())
