#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
MANIFEST="${ROOT}/android/app/src/main/AndroidManifest.xml"

if [[ ! -f "${MANIFEST}" ]]; then
  echo "error: missing AndroidManifest at ${MANIFEST}" >&2
  exit 1
fi

python3 - "${MANIFEST}" <<'PY'
import re
import sys

manifest_path = sys.argv[1]
text = open(manifest_path, encoding="utf-8").read()

forbidden = (
    "io.sentry.send-default-pii",
    "io.sentry.attach-screenshot",
    "io.sentry.attach-view-hierarchy",
)

# Match meta-data blocks that may span multiple lines.
pattern = re.compile(
    r'<meta-data\b([^>]*)/>',
    re.IGNORECASE | re.DOTALL,
)
name_re = re.compile(r'android:name\s*=\s*"([^"]+)"', re.IGNORECASE)
value_re = re.compile(r'android:value\s*=\s*"([^"]+)"', re.IGNORECASE)

failed = False
for match in pattern.finditer(text):
    attrs = match.group(1)
    name_m = name_re.search(attrs)
    value_m = value_re.search(attrs)
    if not name_m or not value_m:
        continue
    name = name_m.group(1)
    value = value_m.group(1)
    if name in forbidden and value.lower() == "true":
        print(f"error: {name} must not be true in {manifest_path}", file=sys.stderr)
        failed = True

if failed:
    sys.exit(1)

print("OK: Sentry privacy meta-data are not set to true in AndroidManifest.xml")
PY
