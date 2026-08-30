"""Operator map pipeline: extract → mapgen → pix → rings → spa → assemble (SP-100).

Glue Option B on the build host. VPS generate is unsupported. Debug
prepare_spa_debug_root is not this path.
"""

from __future__ import annotations

import argparse
import fnmatch
import hashlib
import json
import logging
import os
import re
import shutil
import subprocess
import sys
from urllib.parse import urlparse

from post_generation.assemble_spa_publish_tree import AssembleError
from post_generation.assemble_spa_publish_tree import assemble_spa_publish_tree


logger = logging.getLogger(__name__)

_MODULE_DIR = os.path.dirname(os.path.abspath(__file__))
_TOOLS_PYTHON = os.path.dirname(_MODULE_DIR)
_REPO_ROOT = os.path.dirname(os.path.dirname(_TOOLS_PYTHON))

DEFAULT_INI_PATH = os.path.join(_MODULE_DIR, "var", "etc", "map_pipeline.ini")
EXTRACT_RINGS_SCRIPT = os.path.join(
    _TOOLS_PYTHON, "street_pixels_spike", "extract_admin_place_polygons.py"
)

DEFAULT_MAP_SERIES = "2026.06.28"
DEFAULT_COUNTRIES = "World,Finland_*"
DEFAULT_ISO = "FI"
DEFAULT_THREADS = 4
DEFAULT_POLICY = os.path.join(
    _REPO_ROOT, "data", "street_pixels", "country_policies.json"
)
DEFAULT_BORDERS = os.path.join(_REPO_ROOT, "data", "borders")

WORLD_NAME = "World"
WORLD_COASTS_NAME = "WorldCoasts"

STAGE_MAPGEN = "mapgen"
STAGE_PIX_DERIVE = "pix_derive"
STAGE_RINGS = "rings"
STAGE_SPA_EMIT = "spa_emit"
STAGE_ASSEMBLE = "assemble"
STAGE_RSYNC = "rsync"

CORE_STAGES = (
    STAGE_MAPGEN,
    STAGE_PIX_DERIVE,
    STAGE_RINGS,
    STAGE_SPA_EMIT,
    STAGE_ASSEMBLE,
)
ALL_STAGES = CORE_STAGES + (STAGE_RSYNC,)

COMAPS_MAP_HOSTS = (
    "cdn-us-1.comaps.app",
    "cdn-us-2.comaps.tech",
    "cdn-fi-1.comaps.app",
    "mapgen-fi-1.comaps.app",
    "comaps.firewall-gateway.de",
    "comaps.openstreetmap.fr",
    "comaps-it1.unfoxo.it",
    "comaps-cdn.s3-website.cloud.ru",
    "cdn.comaps.app",
)

ORGANIC_MAPS_SUBWAY_HOST = "cdn.organicmaps.app"


class MapPipelineError(Exception):
    """Operator-facing fail-closed error."""


def load_default_ini_text():
    with open(DEFAULT_INI_PATH, "r", encoding="utf-8") as f:
        return f.read()


def denied_host_in(value):
    if not value:
        return None
    text = str(value).lower()
    for host in COMAPS_MAP_HOSTS:
        if host.lower() in text:
            return host
    return None


def organic_maps_subway_in(value):
    if not value:
        return None
    if ORGANIC_MAPS_SUBWAY_HOST in str(value).lower():
        return ORGANIC_MAPS_SUBWAY_HOST
    return None


def pipeline_stage_names(from_stage=None, rsync_dest=None):
    stages = list(CORE_STAGES)
    if rsync_dest:
        stages.append(STAGE_RSYNC)
    if from_stage:
        if from_stage not in ALL_STAGES:
            raise MapPipelineError(
                "unknown --from-stage {!r}; choose from {}".format(
                    from_stage, ", ".join(ALL_STAGES)
                )
            )
        if from_stage not in stages:
            raise MapPipelineError(
                "--from-stage {!r} requires --rsync-dest".format(from_stage)
            )
        idx = stages.index(from_stage)
        stages = stages[idx:]
    return tuple(stages)


def split_country_selector(line):
    if not line:
        return []
    return [x.strip() for x in line.replace(";", ",").split(",") if x.strip()]


def list_border_leaf_ids(borders_dir):
    names = []
    if not borders_dir or not os.path.isdir(borders_dir):
        return names
    for filename in sorted(os.listdir(borders_dir)):
        path = os.path.join(borders_dir, filename)
        if filename.endswith(".poly") and os.path.isfile(path):
            names.append(filename[: -len(".poly")])
    return names


def expand_countries(selector, borders_dir):
    patterns = split_country_selector(selector)
    if not patterns:
        raise MapPipelineError("empty --countries selector")
    all_names = list_border_leaf_ids(borders_dir)
    all_names.append(WORLD_NAME)
    all_names.append(WORLD_COASTS_NAME)
    matched = []
    used = set()
    unmatched = []
    for pattern in patterns:
        hit = False
        for name in all_names:
            if fnmatch.fnmatchcase(name, pattern):
                hit = True
                if name not in used:
                    matched.append(name)
                    used.add(name)
        if not hit:
            unmatched.append(pattern)
    if unmatched:
        raise MapPipelineError(
            "no countries matched: {} (borders-dir {})".format(
                ", ".join(unmatched), borders_dir or "(missing)"
            )
        )
    explicit_world_coasts = WORLD_COASTS_NAME in patterns
    if not explicit_world_coasts:
        matched = [name for name in matched if name != WORLD_COASTS_NAME]
    return matched


def preflight_skip_coast(skip_coast, expanded_countries):
    expanded = list(expanded_countries)
    if skip_coast and WORLD_NAME in expanded:
        raise MapPipelineError(
            "cannot --skip-coast when World is in the expanded country set "
            "(maps_generator forbids World + skip-coast). Omit World from "
            "--countries, or generate coasts from the extract (default)."
        )
    if skip_coast:
        expanded = [name for name in expanded if name != WORLD_COASTS_NAME]
    return expanded


def normalize_pbf_url(pbf):
    if not pbf or not str(pbf).strip():
        raise MapPipelineError("--pbf is required (file:// or Geofabrik / planet OSM HTTPS)")
    raw = str(pbf).strip()
    parsed = urlparse(raw)
    if parsed.scheme in ("http", "https", "file"):
        return raw
    if parsed.scheme:
        raise MapPipelineError("unsupported --pbf scheme {!r}".format(parsed.scheme))
    return "file://{}".format(os.path.abspath(raw))


def file_url_to_path(url):
    parsed = urlparse(url)
    if parsed.scheme != "file":
        return None
    path = parsed.path or ""
    if parsed.netloc and parsed.netloc != "localhost":
        path = "/{}{}".format(parsed.netloc, path)
    return path


def collect_origin_values(pbf_url, cdn_base, extra_urls, ini_text=None):
    values = []
    if pbf_url:
        values.append(pbf_url)
    if cdn_base:
        if isinstance(cdn_base, (list, tuple)):
            values.extend(cdn_base)
        else:
            values.append(cdn_base)
    for item in extra_urls or ():
        if item:
            values.append(item)
    if ini_text:
        values.append(ini_text)
    return values


def refuse_denied_origins(values, allow_comaps_origin, cdn_base=None):
    if cdn_base and not allow_comaps_origin:
        raise MapPipelineError(
            "--cdn-base is refused unless --allow-comaps-origin "
            "(prepare_spa_debug_root is not the production path)"
        )
    for value in values:
        host = denied_host_in(value)
        if host and not allow_comaps_origin:
            raise MapPipelineError(
                "CoMaps map host {!r} is refused unless --allow-comaps-origin".format(
                    host
                )
            )
        organic = organic_maps_subway_in(value)
        if organic and not allow_comaps_origin:
            raise MapPipelineError(
                "Organic Maps subway host {!r} is refused; provide an independent "
                "SUBWAY_URL or skip subway with a warning".format(organic)
            )


def extra_source_exists(value):
    if value is None:
        return False
    text = str(value).strip()
    if not text:
        return False
    parsed = urlparse(text)
    if parsed.scheme in ("http", "https"):
        return True
    if parsed.scheme == "file":
        path = file_url_to_path(text)
        return bool(path) and os.path.exists(path)
    if os.path.exists(text):
        return True
    return False


def plan_extra_skips(
    hotels_url="",
    ugc_url="",
    subway_url="",
    srtm_path="",
    isolines_path="",
    enable_wikipedia=False,
    reviews_path="",
    popularity_url="",
    transit_url="",
):
    warnings = []
    skip = []
    production = False

    def _missing(label, skip_stage=None):
        msg = (
            "{}: no independent source; skipping with warning "
            "(do not fetch CoMaps map hosts to complete extras)".format(label)
        )
        warnings.append(msg)
        if skip_stage and skip_stage not in skip:
            skip.append(skip_stage)

    if extra_source_exists(hotels_url):
        production = True
    else:
        _missing("hotels")

    if extra_source_exists(ugc_url):
        production = True
    else:
        _missing("ugc", "Ugc")

    if extra_source_exists(subway_url) or extra_source_exists(transit_url):
        pass
    else:
        _missing("subway", "RoutingTransit")

    if extra_source_exists(srtm_path):
        pass
    else:
        _missing("srtm", "Srtm")

    if extra_source_exists(isolines_path):
        pass
    else:
        _missing("isolines", "IsolinesInfo")

    if enable_wikipedia:
        pass
    else:
        warnings.append(
            "wikipedia/descriptions: no local dump; skipping "
            "(pass --enable-wikipedia to download from Wikipedia; "
            "do not fetch CoMaps map hosts)"
        )
        if "DownloadDescriptions" not in skip:
            skip.append("DownloadDescriptions")
        if "Descriptions" not in skip:
            skip.append("Descriptions")
        if "Popularity" not in skip:
            skip.append("Popularity")
        if "PopularityWorld" not in skip:
            skip.append("PopularityWorld")

    if extra_source_exists(reviews_path):
        pass
    else:
        _missing("reviews", "Reviews")

    if extra_source_exists(popularity_url):
        production = True
    elif "Popularity" not in skip:
        _missing("popularity", "Popularity")

    return skip, warnings, production


def set_ini_option(text, key, value):
    replacement = "{}: {}".format(key, value)
    pattern = re.compile(
        r"^([ \t]*#?[ \t]*)" + re.escape(key) + r"[ \t]*:.*$", re.MULTILINE
    )
    if pattern.search(text):
        return pattern.sub(replacement, text, count=1)
    return text.rstrip() + "\n{}\n".format(replacement)


def render_pipeline_ini(
    base_text,
    omim_path,
    build_path,
    main_out_path,
    planet_url,
    planet_md5_url=None,
    threads=DEFAULT_THREADS,
    hotels_url="",
    ugc_url="",
    subway_url="",
    srtm_path="",
    isolines_path="",
    reviews_path="",
    popularity_url="",
    transit_url="",
):
    text = base_text
    text = set_ini_option(text, "OMIM_PATH", omim_path)
    text = set_ini_option(text, "BUILD_PATH", build_path)
    text = set_ini_option(text, "MAIN_OUT_PATH", main_out_path)
    text = set_ini_option(text, "PLANET_URL", planet_url)
    if planet_md5_url:
        text = set_ini_option(text, "PLANET_MD5_URL", planet_md5_url)
    text = set_ini_option(text, "THREADS_COUNT", str(int(threads)))
    text = set_ini_option(text, "THREADS_COUNT_FEATURES_STAGE", str(int(threads)))
    text = set_ini_option(text, "NODE_STORAGE", "map")
    if hotels_url:
        text = set_ini_option(text, "HOTELS_URL", hotels_url)
    if ugc_url:
        text = set_ini_option(text, "UGC_URL", ugc_url)
    if subway_url:
        text = set_ini_option(text, "SUBWAY_URL", subway_url)
    if srtm_path:
        text = set_ini_option(text, "SRTM_PATH", srtm_path)
    if isolines_path:
        text = set_ini_option(text, "ISOLINES_PATH", isolines_path)
    if reviews_path:
        text = set_ini_option(text, "REVIEWS_PATH", reviews_path)
    if popularity_url:
        text = set_ini_option(text, "POPULARITY_URL", popularity_url)
    if transit_url:
        text = set_ini_option(text, "TRANSIT_URL", transit_url)
    return text


def find_build_tool(name, build_path, omim_path):
    found = shutil.which(name)
    if found:
        return found
    roots = []
    if build_path:
        roots.append(build_path)
    if omim_path:
        parent = os.path.dirname(omim_path)
        roots.extend(
            [
                os.path.join(omim_path, "omim-build-release"),
                os.path.join(omim_path, "omim-build-debug"),
                os.path.join(parent, "omim-build-release"),
                os.path.join(parent, "omim-build-debug"),
            ]
        )
    seen = set()
    for root in roots:
        root = os.path.abspath(root)
        if root in seen or not os.path.isdir(root):
            continue
        seen.add(root)
        direct = os.path.join(root, name)
        if os.path.isfile(direct) and os.access(direct, os.X_OK):
            return direct
    return None


def resolve_tool(name, explicit, build_path, omim_path, required):
    if explicit:
        return explicit
    found = find_build_tool(name, build_path, omim_path)
    if found:
        return found
    if required:
        raise MapPipelineError(
            "{} not found under --build-path or PATH (same revision as the APK)".format(
                name
            )
        )
    return name


def write_md5_for_file(path):
    digest = hashlib.md5()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(chunk)
    md5_path = path + ".md5"
    with open(md5_path, "w", encoding="utf-8") as f:
        f.write("{}  {}\n".format(digest.hexdigest(), os.path.basename(path)))
    return md5_path


def ensure_planet_md5_url(pbf_url, planet_md5_url, dry_run):
    if planet_md5_url:
        return planet_md5_url
    local = file_url_to_path(pbf_url)
    if local:
        sibling = local + ".md5"
        if os.path.isfile(sibling):
            return "file://{}".format(os.path.abspath(sibling))
        if dry_run:
            return "file://{}".format(os.path.abspath(sibling))
        if os.path.isfile(local):
            written = write_md5_for_file(local)
            return "file://{}".format(os.path.abspath(written))
        return pbf_url + ".md5"
    return pbf_url + ".md5"


def discover_mwm_dir(mapgen_out):
    if not mapgen_out or not os.path.isdir(mapgen_out):
        return None
    builds = []
    for name in os.listdir(mapgen_out):
        path = os.path.join(mapgen_out, name)
        if os.path.isdir(path):
            builds.append(path)
    builds.sort()
    for build in reversed(builds):
        for name in sorted(os.listdir(build)):
            path = os.path.join(build, name)
            if os.path.isdir(path) and os.path.isfile(
                os.path.join(path, "countries.txt")
            ):
                return path
        for name in sorted(os.listdir(build)):
            path = os.path.join(build, name)
            if os.path.isdir(path):
                for fname in os.listdir(path):
                    if fname.endswith(".mwm"):
                        return path
    return None


def discover_planet_pbf(mapgen_out):
    mwm_dir = discover_mwm_dir(mapgen_out)
    if mwm_dir:
        build = os.path.dirname(mwm_dir)
        candidate = os.path.join(build, "planet.osm.pbf")
        if os.path.isfile(candidate):
            return candidate
    if mapgen_out and os.path.isdir(mapgen_out):
        for root, _dirs, files in os.walk(mapgen_out):
            if "planet.osm.pbf" in files:
                return os.path.join(root, "planet.osm.pbf")
    return None


def read_data_version_from_countries(countries_path):
    with open(countries_path, "r", encoding="utf-8") as f:
        payload = json.load(f)
    return int(payload["v"])


def default_omim_path():
    return _REPO_ROOT


def default_build_path(omim_path):
    for name in ("omim-build-release", "omim-build-debug"):
        path = os.path.join(omim_path, name)
        if os.path.isdir(path):
            return path
        sibling = os.path.join(os.path.dirname(omim_path), name)
        if os.path.isdir(sibling):
            return sibling
    return os.path.join(omim_path, "omim-build-release")


def build_plan(
    pbf,
    out,
    countries=DEFAULT_COUNTRIES,
    map_series=DEFAULT_MAP_SERIES,
    data_version=None,
    iso=DEFAULT_ISO,
    policy=None,
    borders_dir=None,
    secret_key=None,
    from_stage=None,
    skip_coast=False,
    allow_comaps_origin=False,
    rsync_dest=None,
    cdn_base=None,
    work_dir=None,
    omim_path=None,
    build_path=None,
    threads=DEFAULT_THREADS,
    hotels_url="",
    ugc_url="",
    subway_url="",
    srtm_path="",
    isolines_path="",
    enable_wikipedia=False,
    reviews_path="",
    popularity_url="",
    transit_url="",
    planet_md5_url=None,
    pix_derive_bin=None,
    spa_emit_bin=None,
    mwm_dir=None,
    pix_dir=None,
    spa_dir=None,
    rings=None,
    countries_txt=None,
    mapgen_config=None,
    border_prefix="Finland",
    include_mwm=True,
    dry_run=False,
):
    pbf_url = normalize_pbf_url(pbf)
    out_abs = os.path.abspath(out)
    omim = os.path.abspath(omim_path or default_omim_path())
    build = os.path.abspath(build_path or default_build_path(omim))
    borders = os.path.abspath(borders_dir or DEFAULT_BORDERS)
    policy_path = os.path.abspath(policy or DEFAULT_POLICY)
    work = os.path.abspath(work_dir or (out_abs + ".work"))
    mapgen_out = os.path.join(work, "mapgen")
    ini_path = os.path.join(work, "map_pipeline.ini")
    state_path = os.path.join(work, "pipeline_state.json")

    extra_urls = (
        hotels_url,
        ugc_url,
        subway_url,
        srtm_path,
        isolines_path,
        reviews_path,
        popularity_url,
        transit_url,
        planet_md5_url,
        mapgen_config,
    )
    default_ini = load_default_ini_text()
    refuse_denied_origins(
        collect_origin_values(pbf_url, cdn_base, extra_urls, default_ini),
        allow_comaps_origin,
        cdn_base=cdn_base,
    )

    if not os.path.isdir(borders):
        raise MapPipelineError("borders directory not found: {}".format(borders))

    expanded = expand_countries(countries, borders)
    expanded = preflight_skip_coast(skip_coast, expanded)
    if not expanded:
        raise MapPipelineError("expanded country set is empty")

    skip_stages, extra_warnings, production = plan_extra_skips(
        hotels_url=hotels_url,
        ugc_url=ugc_url,
        subway_url=subway_url,
        srtm_path=srtm_path,
        isolines_path=isolines_path,
        enable_wikipedia=enable_wikipedia,
        reviews_path=reviews_path,
        popularity_url=popularity_url,
        transit_url=transit_url,
    )
    if skip_coast and "Coastline" not in skip_stages:
        skip_stages.append("Coastline")
        extra_warnings.append(
            "skip-coast: StageCoastline skipped; WorldCoasts omitted; ocean fill may be missing"
        )

    md5_url = ensure_planet_md5_url(pbf_url, planet_md5_url, dry_run=True)
    if mapgen_config:
        with open(mapgen_config, "r", encoding="utf-8") as f:
            ini_text = f.read()
        refuse_denied_origins([ini_text], allow_comaps_origin)
    else:
        ini_text = render_pipeline_ini(
            default_ini,
            omim_path=omim,
            build_path=build,
            main_out_path=mapgen_out,
            planet_url=pbf_url,
            planet_md5_url=md5_url,
            threads=threads,
            hotels_url=hotels_url,
            ugc_url=ugc_url,
            subway_url=subway_url,
            srtm_path=srtm_path,
            isolines_path=isolines_path,
            reviews_path=reviews_path,
            popularity_url=popularity_url,
            transit_url=transit_url,
        )
    refuse_denied_origins([ini_text], allow_comaps_origin)

    stages = pipeline_stage_names(from_stage=from_stage, rsync_dest=rsync_dest)
    pix_required = STAGE_PIX_DERIVE in stages and not dry_run
    spa_required = STAGE_SPA_EMIT in stages and not dry_run
    pix_bin = resolve_tool(
        "pix_derive_tool", pix_derive_bin, build, omim, required=pix_required
    )
    spa_bin = resolve_tool(
        "spa_emit_tool", spa_emit_bin, build, omim, required=spa_required
    )

    default_pix = os.path.join(work, "pix")
    default_spa = os.path.join(work, "spa")
    default_rings = os.path.join(work, "rings.jsonl")
    pix_out = os.path.abspath(pix_dir) if pix_dir else default_pix
    spa_out = os.path.abspath(spa_dir) if spa_dir else default_spa
    rings_out = os.path.abspath(rings) if rings else default_rings
    local_pbf = file_url_to_path(pbf_url)

    countries_arg = ",".join(expanded)
    return {
        "stages": list(stages),
        "countries": list(expanded),
        "countries_arg": countries_arg,
        "countries_selector": countries,
        "pbf_url": pbf_url,
        "pbf_path": local_pbf,
        "out": out_abs,
        "work_dir": work,
        "mapgen_out": mapgen_out,
        "ini_path": ini_path,
        "ini_text": ini_text,
        "state_path": state_path,
        "map_series": map_series,
        "data_version": data_version,
        "iso": iso,
        "policy": policy_path,
        "borders_dir": borders,
        "secret_key": secret_key,
        "skip_coast": bool(skip_coast),
        "omit_world_coasts": WORLD_COASTS_NAME not in expanded,
        "mapgen_skip": list(skip_stages),
        "mapgen_production": bool(production),
        "extra_warnings": list(extra_warnings),
        "threads": int(threads),
        "omim_path": omim,
        "build_path": build,
        "pix_derive_bin": pix_bin,
        "spa_emit_bin": spa_bin,
        "mwm_dir": os.path.abspath(mwm_dir) if mwm_dir else None,
        "pix_dir": pix_out,
        "spa_dir": spa_out,
        "rings": rings_out,
        "countries_txt": os.path.abspath(countries_txt) if countries_txt else None,
        "border_prefix": border_prefix,
        "include_mwm": bool(include_mwm),
        "rsync_dest": rsync_dest,
        "extract_rings_script": EXTRACT_RINGS_SCRIPT,
        "planet_md5_url": md5_url,
        "mapgen_config": mapgen_config,
        "dry_run": bool(dry_run),
        "node_storage": "map",
        "vps_generate_unsupported": True,
    }


def print_plan(plan):
    print("Street Pixels map_pipeline (SP-100)")
    print("  stages: {}".format(" → ".join(plan["stages"])))
    print("  pbf: {}".format(plan["pbf_url"]))
    print("  countries selector: {}".format(plan["countries_selector"]))
    print("  expanded countries: {}".format(",".join(plan["countries"])))
    print("  omit WorldCoasts: {}".format(plan["omit_world_coasts"]))
    print("  skip-coast: {}".format(plan["skip_coast"]))
    print("  NODE_STORAGE: {}".format(plan["node_storage"]))
    print("  THREADS_COUNT: {}".format(plan["threads"]))
    print("  mapgen --production: {}".format(plan["mapgen_production"]))
    print("  mapgen --skip: {}".format(",".join(plan["mapgen_skip"]) or "(none)"))
    print("  out: {}".format(plan["out"]))
    print("  work_dir: {}".format(plan["work_dir"]))
    print("  ini: {}".format(plan["ini_path"]))
    print("  mapgen_out: {}".format(plan["mapgen_out"]))
    print("  mwm_dir: {}".format(plan["mwm_dir"] or "(after mapgen)"))
    print("  pix_dir: {}".format(plan["pix_dir"]))
    print("  rings: {}".format(plan["rings"]))
    print("  spa_dir: {}".format(plan["spa_dir"]))
    print("  policy: {}".format(plan["policy"]))
    print("  borders: {}".format(plan["borders_dir"]))
    print("  map_series: {}".format(plan["map_series"]))
    print("  data_version: {}".format(plan["data_version"] or "(from countries.txt)"))
    print("  pix_derive_tool: {}".format(plan["pix_derive_bin"]))
    print("  spa_emit_tool: {}".format(plan["spa_emit_bin"]))
    print("  rsync_dest: {}".format(plan["rsync_dest"] or "(none)"))
    print("  VPS generate: unsupported (SPD-088); this CLI is build-host only")
    if plan["dry_run"]:
        print("  dry-run: no subprocess, no network")
    for warning in plan["extra_warnings"]:
        print("warning: {}".format(warning))


def run_command(argv, cwd=None, env=None):
    logger.info("run: %s", " ".join(str(x) for x in argv))
    completed = subprocess.run(argv, cwd=cwd, env=env)
    if completed.returncode != 0:
        raise MapPipelineError(
            "command failed ({}) : {}".format(
                completed.returncode, " ".join(str(x) for x in argv)
            )
        )
    return completed.returncode


def tools_python_env():
    env = os.environ.copy()
    existing = env.get("PYTHONPATH", "")
    parts = [_TOOLS_PYTHON]
    if existing:
        parts.append(existing)
    env["PYTHONPATH"] = os.pathsep.join(parts)
    return env


def write_text(path, text):
    parent = os.path.dirname(path)
    if parent:
        os.makedirs(parent, exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)


def load_state(state_path):
    if not state_path or not os.path.isfile(state_path):
        return {}
    with open(state_path, "r", encoding="utf-8") as f:
        return json.load(f)


def save_state(state_path, state):
    write_text(state_path, json.dumps(state, indent=2, sort_keys=True) + "\n")


def resolve_runtime_paths(plan):
    state = load_state(plan["state_path"])
    mwm_dir = plan["mwm_dir"] or state.get("mwm_dir")
    if not mwm_dir:
        mwm_dir = discover_mwm_dir(plan["mapgen_out"])
    countries_txt = plan["countries_txt"] or state.get("countries_txt")
    if not countries_txt and mwm_dir:
        candidate = os.path.join(mwm_dir, "countries.txt")
        if os.path.isfile(candidate):
            countries_txt = candidate
    pbf_path = plan["pbf_path"] or state.get("pbf_path")
    if not pbf_path:
        pbf_path = discover_planet_pbf(plan["mapgen_out"])
    data_version = plan["data_version"] or state.get("data_version")
    if data_version is None and countries_txt and os.path.isfile(countries_txt):
        data_version = read_data_version_from_countries(countries_txt)
    return {
        "mwm_dir": mwm_dir,
        "countries_txt": countries_txt,
        "pbf_path": pbf_path,
        "data_version": data_version,
        "pix_dir": plan["pix_dir"] or state.get("pix_dir"),
        "spa_dir": plan["spa_dir"] or state.get("spa_dir"),
        "rings": plan["rings"] or state.get("rings"),
    }


def run_mapgen(plan):
    os.makedirs(plan["mapgen_out"], exist_ok=True)
    ini_text = plan["ini_text"]
    if not plan.get("mapgen_config"):
        md5_url = ensure_planet_md5_url(
            plan["pbf_url"], plan.get("planet_md5_url"), dry_run=False
        )
        ini_text = set_ini_option(ini_text, "PLANET_MD5_URL", md5_url)
    write_text(plan["ini_path"], ini_text)
    argv = [
        sys.executable,
        "-m",
        "maps_generator",
        "--config",
        plan["ini_path"],
        "--countries",
        plan["countries_arg"],
        "--suffix",
        "sp100",
    ]
    if plan["mapgen_production"]:
        argv.append("--production")
    if plan["mapgen_skip"]:
        argv.extend(["--skip", ",".join(plan["mapgen_skip"])])
    run_command(argv, cwd=_TOOLS_PYTHON, env=tools_python_env())
    mwm_dir = discover_mwm_dir(plan["mapgen_out"])
    if not mwm_dir:
        raise MapPipelineError(
            "maps_generator finished but no MWM directory with countries.txt was found under {}".format(
                plan["mapgen_out"]
            )
        )
    countries_txt = os.path.join(mwm_dir, "countries.txt")
    data_version = plan["data_version"]
    if os.path.isfile(countries_txt):
        data_version = read_data_version_from_countries(countries_txt)
    pbf_path = discover_planet_pbf(plan["mapgen_out"]) or plan["pbf_path"]
    state = {
        "mwm_dir": mwm_dir,
        "countries_txt": countries_txt,
        "data_version": data_version,
        "pbf_path": pbf_path,
        "pix_dir": plan["pix_dir"],
        "spa_dir": plan["spa_dir"],
        "rings": plan["rings"],
    }
    save_state(plan["state_path"], state)
    return state


def run_pix_derive(plan, runtime):
    mwm_dir = runtime["mwm_dir"]
    if not mwm_dir or not os.path.isdir(mwm_dir):
        raise MapPipelineError(
            "pix_derive requires an MWM directory (run mapgen or pass --mwm-dir)"
        )
    os.makedirs(plan["pix_dir"], exist_ok=True)
    argv = [
        plan["pix_derive_bin"],
        "--mwm_dir",
        mwm_dir,
        "--out_dir",
        plan["pix_dir"],
    ]
    if runtime["data_version"] is not None:
        argv.extend(["--map_data_version", str(int(runtime["data_version"]))])
    run_command(argv)


def run_rings(plan, runtime):
    pbf_path = runtime["pbf_path"] or plan["pbf_path"]
    if not pbf_path or not os.path.isfile(pbf_path):
        raise MapPipelineError(
            "rings extract requires a local PBF (file:// --pbf or planet.osm.pbf after mapgen)"
        )
    parent = os.path.dirname(plan["rings"])
    if parent:
        os.makedirs(parent, exist_ok=True)
    argv = [
        sys.executable,
        plan["extract_rings_script"],
        "--pbf",
        pbf_path,
        "--out-jsonl",
        plan["rings"],
    ]
    run_command(argv, cwd=_TOOLS_PYTHON, env=tools_python_env())


def run_spa_emit(plan, runtime):
    if not os.path.isfile(plan["rings"]):
        raise MapPipelineError("rings JSONL not found: {}".format(plan["rings"]))
    if not os.path.isdir(plan["pix_dir"]):
        raise MapPipelineError("pix_dir not found: {}".format(plan["pix_dir"]))
    os.makedirs(plan["spa_dir"], exist_ok=True)
    data_version = runtime["data_version"]
    if data_version is None:
        raise MapPipelineError("--data-version is required for spa_emit when countries.txt is absent")
    argv = [
        plan["spa_emit_bin"],
        "--mode=production",
        "--rings={}".format(plan["rings"]),
        "--policy={}".format(plan["policy"]),
        "--iso={}".format(plan["iso"]),
        "--out_dir={}".format(plan["spa_dir"]),
        "--borders_dir={}".format(plan["borders_dir"]),
        "--pix_dir={}".format(plan["pix_dir"]),
        "--map_data_version={}".format(int(data_version)),
    ]
    if plan["border_prefix"]:
        argv.append("--border_prefix={}".format(plan["border_prefix"]))
    run_command(argv)


def run_assemble(plan, runtime):
    countries_txt = runtime["countries_txt"]
    if not countries_txt or not os.path.isfile(countries_txt):
        raise MapPipelineError(
            "assemble requires countries.txt from mapgen (or --countries-txt)"
        )
    data_version = runtime["data_version"]
    if data_version is None:
        data_version = read_data_version_from_countries(countries_txt)
    mwm_dir = runtime["mwm_dir"] if plan["include_mwm"] else None
    rc = assemble_spa_publish_tree(
        countries_path=countries_txt,
        spa_dir=plan["spa_dir"],
        out=plan["out"],
        map_series=plan["map_series"],
        data_version=int(data_version),
        mwm_dir=mwm_dir,
        secret_key=plan["secret_key"],
        include_mwm=plan["include_mwm"],
        spa_only=not plan["include_mwm"],
    )
    if rc != 0:
        raise MapPipelineError("assemble_spa_publish_tree failed with code {}".format(rc))


def run_rsync(plan):
    dest = plan["rsync_dest"]
    if not dest:
        raise MapPipelineError("--rsync-dest is required for the rsync stage")
    src = plan["out"].rstrip("/") + "/"
    argv = ["rsync", "-a", src, dest]
    run_command(argv)


STAGE_RUNNERS = {
    STAGE_MAPGEN: lambda plan, runtime: run_mapgen(plan),
    STAGE_PIX_DERIVE: run_pix_derive,
    STAGE_RINGS: run_rings,
    STAGE_SPA_EMIT: run_spa_emit,
    STAGE_ASSEMBLE: run_assemble,
    STAGE_RSYNC: lambda plan, runtime: run_rsync(plan),
}


def run_map_pipeline(**kwargs):
    dry_run = bool(kwargs.pop("dry_run", False))
    plan = build_plan(dry_run=dry_run, **kwargs)
    print_plan(plan)
    if dry_run:
        return plan
    os.makedirs(plan["work_dir"], exist_ok=True)
    write_text(plan["ini_path"], plan["ini_text"])
    runtime = resolve_runtime_paths(plan)
    for stage in plan["stages"]:
        runner = STAGE_RUNNERS[stage]
        logger.info("stage %s: start", stage)
        runner(plan, runtime)
        runtime = resolve_runtime_paths(plan)
        logger.info("stage %s: done", stage)
    print("OK: {}".format(plan["out"]))
    return plan


def build_arg_parser():
    parser = argparse.ArgumentParser(
        description=(
            "Build-host Street Pixels map pipeline (SP-100). Stages: "
            "mapgen → pix_derive → rings → spa_emit → assemble "
            "(optional rsync last). Writes an SP-050 --out tree. "
            "VPS generate is unsupported (SPD-088). "
            "prepare_spa_debug_root is not this path (SPD-087)."
        )
    )
    parser.add_argument(
        "--pbf",
        required=True,
        help="OSM extract as file:// path or Geofabrik / planet OSM HTTPS URL",
    )
    parser.add_argument("--out", required=True, help="SP-050 publish tree root")
    parser.add_argument(
        "--countries",
        default=DEFAULT_COUNTRIES,
        help="Country/leaf selector (default {})".format(DEFAULT_COUNTRIES),
    )
    parser.add_argument(
        "--map-series",
        default=DEFAULT_MAP_SERIES,
        help="MAP_SERIES (default {})".format(DEFAULT_MAP_SERIES),
    )
    parser.add_argument(
        "--data-version",
        type=int,
        default=None,
        help="map_data_version / countries v (default: read from generated countries.txt)",
    )
    parser.add_argument("--iso", default=DEFAULT_ISO, help="ISO 3166-1 alpha-2 (default FI)")
    parser.add_argument("--policy", default=None, help="country_policies.json path")
    parser.add_argument("--borders-dir", default=None, help="data/borders directory")
    parser.add_argument("--secret-key", default=None, help="Optional Ed25519 PEM for countries.txt.sig")
    parser.add_argument(
        "--from-stage",
        choices=list(ALL_STAGES),
        default=None,
        help="Skip earlier pipeline stages (mapgen, pix_derive, rings, spa_emit, assemble, rsync)",
    )
    parser.add_argument(
        "--skip-coast",
        action="store_true",
        help="Skip maps_generator Coastline (error if World is in the expanded set)",
    )
    parser.add_argument(
        "--allow-comaps-origin",
        action="store_true",
        help="Allow CoMaps map hosts / --cdn-base (default off)",
    )
    parser.add_argument(
        "--cdn-base",
        action="append",
        default=None,
        help="Refused unless --allow-comaps-origin (not the production path)",
    )
    parser.add_argument("--rsync-dest", default=None, help="Optional rsync destination of --out")
    parser.add_argument("--dry-run", action="store_true", help="Print the graph and paths; no network")
    parser.add_argument("--work-dir", default=None, help="Scratch dir (default {out}.work)")
    parser.add_argument("--omim-path", default=None, help="Repository root (generator revision)")
    parser.add_argument("--build-path", default=None, help="Directory with generator_tool / pix_derive_tool / spa_emit_tool")
    parser.add_argument("--threads", type=int, default=DEFAULT_THREADS, help="THREADS_COUNT cap (default 4, not 0=all cores)")
    parser.add_argument("--hotels-url", default="", help="Independent hotels feed (empty → skip with warning)")
    parser.add_argument("--ugc-url", default="", help="Independent UGC feed (empty → skip with warning)")
    parser.add_argument("--subway-url", default="", help="Independent subway JSON (empty → skip with warning)")
    parser.add_argument("--srtm-path", default="", help="Local SRTM directory (empty → skip with warning)")
    parser.add_argument("--isolines-path", default="", help="Local isolines directory (empty → skip with warning)")
    parser.add_argument(
        "--enable-wikipedia",
        action="store_true",
        help="Allow maps_generator Wikipedia/descriptions download (independent; default skip with warning)",
    )
    parser.add_argument("--reviews-path", default="", help="Local reviews file (empty → skip with warning)")
    parser.add_argument("--popularity-url", default="", help="Independent popularity feed")
    parser.add_argument("--transit-url", default="", help="Independent GTFS transit feed")
    parser.add_argument("--planet-md5-url", default=None, help="Override PLANET_MD5_URL (default {pbf}.md5)")
    parser.add_argument("--pix-derive-bin", default=None, help="pix_derive_tool path")
    parser.add_argument("--spa-emit-bin", default=None, help="spa_emit_tool path")
    parser.add_argument("--mwm-dir", default=None, help="Existing MWM dir when skipping mapgen")
    parser.add_argument("--pix-dir", default=None, help="Override pix output / input directory")
    parser.add_argument("--spa-dir", default=None, help="Override spa output / input directory")
    parser.add_argument("--rings", default=None, help="Override rings JSONL path")
    parser.add_argument("--countries-txt", default=None, help="Existing countries.txt when skipping mapgen")
    parser.add_argument("--mapgen-config", default=None, help="Existing maps_generator ini (still origin-checked)")
    parser.add_argument("--border-prefix", default="Finland", help="spa_emit_tool --border_prefix (default Finland)")
    parser.add_argument(
        "--include-mwm",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Copy MWMs into the SP-050 tree (default true)",
    )
    parser.add_argument("--verbose", action="store_true")
    return parser


def main(argv=None):
    args = build_arg_parser().parse_args(argv)
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(levelname)s %(message)s",
    )
    try:
        run_map_pipeline(
            pbf=args.pbf,
            out=args.out,
            countries=args.countries,
            map_series=args.map_series,
            data_version=args.data_version,
            iso=args.iso,
            policy=args.policy,
            borders_dir=args.borders_dir,
            secret_key=args.secret_key,
            from_stage=args.from_stage,
            skip_coast=args.skip_coast,
            allow_comaps_origin=args.allow_comaps_origin,
            rsync_dest=args.rsync_dest,
            cdn_base=args.cdn_base,
            work_dir=args.work_dir,
            omim_path=args.omim_path,
            build_path=args.build_path,
            threads=args.threads,
            hotels_url=args.hotels_url,
            ugc_url=args.ugc_url,
            subway_url=args.subway_url,
            srtm_path=args.srtm_path,
            isolines_path=args.isolines_path,
            enable_wikipedia=args.enable_wikipedia,
            reviews_path=args.reviews_path,
            popularity_url=args.popularity_url,
            transit_url=args.transit_url,
            planet_md5_url=args.planet_md5_url,
            pix_derive_bin=args.pix_derive_bin,
            spa_emit_bin=args.spa_emit_bin,
            mwm_dir=args.mwm_dir,
            pix_dir=args.pix_dir,
            spa_dir=args.spa_dir,
            rings=args.rings,
            countries_txt=args.countries_txt,
            mapgen_config=args.mapgen_config,
            border_prefix=args.border_prefix,
            include_mwm=args.include_mwm,
            dry_run=args.dry_run,
        )
    except (MapPipelineError, AssembleError) as exc:
        logger.error("%s", exc)
        print("error: {}".format(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
