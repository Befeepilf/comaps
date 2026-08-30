# SP-101 — Map identity (keys, stock hosts, World bootstrap)

**Status:** Recipe for maintainers. Secrets stay gitignored.
**Decisions:** SPD-036, SPD-037, SPD-087, SPD-091, SPD-092, SPD-093.
**Work item:** [SP-101-independent-map-identity.md](../work-items/SP-101-independent-map-identity.md).
**Tutorial:** [STREET_PIXELS_MAP_PIPELINE.md](../../STREET_PIXELS_MAP_PIPELINE.md).

Street Pixels stock builds must not use Streifzug map CDNs or the Streifzug
`COUNTRIES_TXT_SIGNATURE_HEX`. Channel A still verifies Ed25519
(`openssl pkeyutl -sign -rawin`, same as `maps_generator.utils.file.sign_file`).
Do **not** skip verification when the origin is ours (**SPD-091**).

---

## `private.h` fields (gitignored and untracked)

`private.h` is listed in `.gitignore` **and is not tracked**. A tracked
header previously shipped Streifzug `METASERVER_URL` / `DEFAULT_URLS_JSON`
(including `mapgen-fi-1.streifzug.app`) and Streifzug `COUNTRIES_TXT_SIGNATURE_HEX`;
gitignore does not apply to tracked files. Clones do not receive `private.h`.

When `private.h` is missing:

- `configure.sh` runs `python3 -m street_pixels.map_identity ensure-private-h`
  (after `export PYTHONPATH="$REPO_ROOT/tools/python"`).
- Root `CMakeLists.txt` copies the example at configure time. Android Gradle
  uses that CMakeLists and does not run `configure.sh`.

Neither path overwrites an existing `private.h`. Do not commit `private.h` or
production Ed25519 secret PEMs.

Copy [private.h.street-pixels.example](../../../private.h.street-pixels.example)
to gitignored `private.h` on the build host (or let configure/CMake copy it)
and replace placeholders.

| Macro | Street Pixels stock meaning |
| --- | --- |
| `DEFAULT_URLS_JSON` | HTTPS origin list the APK uses when the metaserver is empty. Template: `https://maps.example.invalid/`. Public host is **SP-102**. Not a Custom Maps LAN URL (D12). |
| `METASERVER_URL` | Empty `""` (template) or our meta host. Empty → client uses `DEFAULT_URLS_JSON` only. |
| `COUNTRIES_TXT_SIGNATURE_HEX` | 64 hex chars = 32-byte Ed25519 **public** key. Template zeros are not a production key. |
| `MAP_SERIES` | Stay `2026.06.28` (**SPD-092**) unless compatibility requires a bump. |

OSM OAuth client IDs in the template are the existing in-tree Streifzug/OSM
values. Do not put production Ed25519 secret PEMs or a live `private.h` in git.

Placeholder origin `https://maps.example.invalid/` is RFC 2606 `.invalid`.
Do not replace it with `http://192.168…` or `10.…` (SP-004).

---

## Keygen (Ed25519 PEM, matches `sign_file`)

`maps_generator.utils.file.sign_file` / `verify_file` run:

```text
openssl pkeyutl -sign  -inkey <secret.pem> -rawin -in <file> -out <file>.sig
openssl pkeyutl -verify -pubin -inkey <public.pem> -rawin -in <file> -sigfile <file>.sig
```

SP-050 `assemble_spa_publish_tree --secret-key` and SP-100
`map_pipeline --secret-key` pass that **secret PEM** to `sign_file`.
The APK verifies with `COUNTRIES_TXT_SIGNATURE_HEX`, not the PEM path.

### Generate a throwaway or production pair

```bash
openssl genpkey -algorithm Ed25519 -out countries_ed25519_secret.pem
openssl pkey -in countries_ed25519_secret.pem -pubout -out countries_ed25519_public.pem
```

Keep `countries_ed25519_secret.pem` on the build host only. Never commit it.

### Extract 64-char public hex for `COUNTRIES_TXT_SIGNATURE_HEX`

Ed25519 SubjectPublicKeyInfo DER is 44 bytes; the last 32 bytes are the raw
public key:

```bash
openssl pkey -pubin -in countries_ed25519_public.pem -outform DER \
  | tail -c 32 | xxd -p -c 32
```

Same helper (no production key required):

```bash
cd tools/python
PYTHONPATH=. python3 -m street_pixels.map_identity public-hex \
  --public-key /path/to/countries_ed25519_public.pem
```

Paste the 64 hex characters into `private.h`:

```c
#define COUNTRIES_TXT_SIGNATURE_HEX "<64 hex chars>"
```

### Sign and check a test `countries.txt`

```bash
cd tools/python
PYTHONPATH=. python3 - <<'PY'
from street_pixels.map_identity import sign_rawin, verify_rawin, ed25519_public_key_hex
sig = sign_rawin("/path/to/countries.txt", "/path/to/countries_ed25519_secret.pem")
assert verify_rawin("/path/to/countries.txt", sig, "/path/to/countries_ed25519_public.pem")
print(ed25519_public_key_hex("/path/to/countries_ed25519_public.pem"))
PY
```

`sign_rawin` uses the same `openssl pkeyutl -sign -rawin` argv as `sign_file`.

A unit test with a **throwaway** key lives in
`tools/python/street_pixels/tests/test_map_identity.py`. Do not put production
keys in CI.

---

## Channel A vs Channel B

Long-term stock path is **Channel A**: signed `countries.txt` on the public
origin. Channel B is debug-only local inject. Recipes:
[spa-advertise-channels.md](spa-advertise-channels.md).

Do not merge spa-bearing `data/countries.txt` until the stock URL serves
blobs (**SPD-037** / SP-103). Custom Maps URL remains a user Advanced
override (D12); do not bake a LAN Custom Maps URL into `DEFAULT_URLS_JSON`.

---

## `configure.sh` World bootstrap

`configure.sh` exports `PYTHONPATH="$REPO_ROOT/tools/python"` then calls
`python3 -m street_pixels.map_identity ensure-private-h` and
`configure-world`. Without that `PYTHONPATH`, `python3 -m street_pixels.map_identity`
from the repo root fails (`No module named 'street_pixels'`). It does **not**
default to `mapgen-fi-1.streifzug.app`. Streifzug map hosts are refused. WorldCoasts
is optional (**SPD-094**): a 404 or missing local coasts file omits coasts;
configure does not fall back to Streifzug.

| Input | Effect |
| --- | --- |
| `SKIP_MAP_DOWNLOAD=1` (or `./configure.sh -m`) | Skip World fetch/copy. Same flag `build_omim.sh` already passes for targeted builds. |
| Existing `data/world_mwm/<v>/World.mwm` or `data/World.mwm` | Keep; no download. |
| `STREET_PIXELS_LOCAL_WORLD=/path/to/World.mwm` | Copy into `data/world_mwm/<v>/` and symlink `data/World.mwm`. Sibling `WorldCoasts.mwm` copied if present. |
| `STREET_PIXELS_WORLD_DIR=/dir` | Copy `World.mwm` from that directory (or `world_mwm/<v>/` under it). |
| `STREET_PIXELS_MAPS_BASE_URL=https://<our-origin>/` | Fetch `{base}/maps/{MAP_SERIES}/{v}/World.mwm` (SPD-035 layout). Must be HTTPS, not a Streifzug host, not a private-range IP. Public origin recipe: [sp-102-publish-and-serve-origin.md](sp-102-publish-and-serve-origin.md). Git template remains `https://maps.example.invalid/`. |
| `MAPS_BASE_URL` | Legacy alias used only when `STREET_PIXELS_MAPS_BASE_URL` is empty. Streifzug values are refused; there is no mapgen-fi-1 fallback. |
| None of the above, and no local World | **Error** with SPD-087 instructions. |

Example (developers without an origin yet):

```bash
SKIP_MAP_DOWNLOAD=1 ./configure.sh
```

Example (operator World from the map pipeline out tree):

```bash
STREET_PIXELS_WORLD_DIR=/path/to/mwm_dir ./configure.sh
```

---

## Egress note

App map downloads use the Street Pixels origin in `private.h`, not Streifzug
CDNs. Geofabrik / planet.openstreetmap.org remain **build-host** extract
sources for SP-100 mapgen, not APK egress. See
[baseline.md §8](../baseline.md#8-network-egress-inventory-sp-004).
