#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/areas_reader.hpp"
#include "street_pixels_areas/sample_centres.hpp"
#include "street_pixels_areas/spa_jsonl.hpp"
#include "street_pixels_areas/subdivision_assignment.hpp"

#include "street_pixels_config/country_config.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/timer.hpp"

#include "defines.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <gflags/gflags.h>

DEFINE_string(mode, "production",
              "Emit mode: production (dense assign from leaf .pix; default) or "
              "geometry_only (fixtures/debug; assign_count=0)");
DEFINE_string(rings, "", "Path to SP-023 finland_admin_place_rings.jsonl");
DEFINE_string(policy, "", "Path to country_policies.json (default: ResourcesDir/street_pixels/...)");
DEFINE_string(iso, "FI", "ISO 3166-1 alpha-2 country code");
DEFINE_string(out_dir, "/tmp/sp044", "Output / publish directory for .spa files (not committed)");
DEFINE_string(borders_dir, "",
              "Directory of leaf .poly borders (production). Default: ResourcesDir/../../data/borders "
              "resolved via --borders_dir explicitly in docs.");
DEFINE_string(pix_dir, "", "Directory of leaf {mwmLeafId}.pix files (production; preferred U source)");
DEFINE_string(leaf, "", "Optional single leaf id filter (production). Empty = all matching borders.");
DEFINE_string(border_prefix, "Finland", "Leaf .poly name prefix filter (empty = all .poly in borders_dir)");
DEFINE_string(helsinki_poly, "",
              "Deprecated SP-032 path: optional Helsinki .poly when mode=geometry_only");
DEFINE_string(helsinki_leaf, "Finland_Southern Finland_Helsinki", "Helsinki MWM leaf id");
DEFINE_string(country_leaf, "Finland", "Country-concat measurement leaf id (geometry_only only)");
DEFINE_int64(map_data_version, 260802, "map_data_version stamped into .spa headers");
DEFINE_bool(verify_dense, true, "Run VerifyDenseAssignments after each dense leaf emit");

namespace
{
using namespace street_pixels;

std::string DefaultPolicyPath()
{
  return base::JoinPath(GetPlatform().ResourcesDir(), kCountryPoliciesRelativePath);
}

void PrintSizes(std::string const & label, SpaSectionSizes const & sizes, uint32_t areaCount,
                uint32_t assignCount)
{
  std::cout << label << " file_bytes=" << sizes.m_fileBytes << " (~" << (sizes.m_fileBytes / (1024.0 * 1024.0))
            << " MiB) hdr=" << sizes.m_hdrBytes << " areas=" << sizes.m_areasBytes
            << " assign=" << sizes.m_assignBytes << " area_count=" << areaCount
            << " assign_count=" << assignCount << "\n";
}

bool SpotCheckHelsinki(std::vector<ExplorationArea> const & areas)
{
  auto const checks = SpotCheckKnownIds(areas, HelsinkiKnownOsmIds());
  uint32_t found = 0;
  uint32_t nameOk = 0;
  std::cout << "  helsinki_known_id_spot_check:\n";
  for (auto const & row : checks)
  {
    if (row.m_found)
      ++found;
    if (row.m_nameMatches)
      ++nameOk;
    std::cout << "    osm_id=" << row.m_osmId << " hint=" << row.m_expectedNameHint
              << " found=" << (row.m_found ? "yes" : "no");
    if (row.m_found)
    {
      std::cout << " name=" << row.m_actualName << " name_match=" << (row.m_nameMatches ? "yes" : "no")
                << " role=" << DebugPrint(row.m_role) << " admin_level=" << static_cast<int>(row.m_adminLevel);
    }
    std::cout << "\n";
  }
  std::cout << "  known_ids_found=" << found << "/" << checks.size() << " name_match=" << nameOk << "/"
            << checks.size() << "\n";
  return found == checks.size() && nameOk == checks.size();
}

void PrintLoadedHeader(SpaFile const & loaded)
{
  std::cout << "  header format_version=" << loaded.m_header.m_formatVersion
            << " nside=" << loaded.m_header.m_nside
            << " universe_order=" << static_cast<int>(loaded.m_header.m_universeOrder)
            << " area_count=" << loaded.m_header.m_areaCount
            << " assign_count=" << loaded.m_header.m_assignCount
            << " policy_version=" << loaded.m_header.m_policyVersion
            << " map_data_version=" << loaded.m_header.m_mapDataVersion
            << " mwm_id=" << loaded.m_header.m_mwmId << "\n";
}

bool EmitGeometryOnly(std::string const & outPath, std::vector<ExplorationArea> areas,
                      CountryPolicy const & policy, SpaWriteParams const & params, JsonlFilterStats const & stats,
                      bool doSpotCheck)
{
  WriteGeometryOnlyExplorationSidecar(outPath, areas, policy, params);
  auto const loaded = ReadExplorationSidecar(outPath);
  auto const sizes = MeasureSpaSectionSizes(outPath);

  std::cout << "wrote " << outPath << " (geometry_only)\n";
  std::cout << "  filter records=" << stats.m_records << " admitted=" << stats.m_admitted;
  for (auto const & [reason, count] : stats.m_rejects)
    std::cout << " " << reason << "=" << count;
  std::cout << "\n";
  PrintLoadedHeader(loaded);
  PrintSizes("  sections", sizes, loaded.m_header.m_areaCount, loaded.m_header.m_assignCount);

  if (!doSpotCheck)
    return true;
  return SpotCheckHelsinki(loaded.m_areas);
}

bool EmitDenseLeaf(std::string const & outPath, std::vector<ExplorationArea> areas,
                   std::vector<m2::PointD> const & sampleCentres, CountryPolicy const & policy,
                   SpaWriteParams const & params, JsonlFilterStats const & stats, bool doSpotCheck,
                   bool verifyDense)
{
  if (sampleCentres.empty())
  {
    LOG(LERROR, ("Production dense emit requires non-empty universe sample centres:", outPath));
    return false;
  }

  WriteExplorationSidecar(outPath, areas, sampleCentres, policy, params);
  auto const loaded = ReadExplorationSidecar(outPath);
  auto const sizes = MeasureSpaSectionSizes(outPath);

  std::cout << "wrote " << outPath << " (production dense)\n";
  std::cout << "  filter records=" << stats.m_records << " admitted=" << stats.m_admitted;
  for (auto const & [reason, count] : stats.m_rejects)
    std::cout << " " << reason << "=" << count;
  std::cout << "\n";
  PrintLoadedHeader(loaded);
  PrintSizes("  sections", sizes, loaded.m_header.m_areaCount, loaded.m_header.m_assignCount);

  if (loaded.m_header.m_formatVersion != kSpaFormatVersion || loaded.m_header.m_nside != kSpaNside ||
      loaded.m_header.m_universeOrder != kSpaUniverseOrderAscendingNest)
  {
    LOG(LERROR, ("Dense emit header does not match SPD-034 freeze", outPath));
    return false;
  }
  if (loaded.m_header.m_assignCount != sampleCentres.size() || loaded.m_header.m_assignCount == 0)
  {
    LOG(LERROR, ("assign_count must equal |U| and be > 0", loaded.m_header.m_assignCount, sampleCentres.size()));
    return false;
  }

  // Settlements must never appear as assign targets.
  for (uint32_t idx : loaded.m_assignments)
  {
    if (idx == NoSubdivisionSentinel(loaded.m_header.m_indexWidth))
      continue;
    if (idx >= loaded.m_areas.size() || !loaded.m_areas[idx].IsAssignable())
    {
      LOG(LERROR, ("Assign column points at non-assignable area", idx));
      return false;
    }
  }

  if (verifyDense && !VerifyDenseAssignments(loaded, sampleCentres, policy))
  {
    LOG(LERROR, ("VerifyDenseAssignments failed for", outPath));
    return false;
  }

  if (!doSpotCheck)
    return true;
  return SpotCheckHelsinki(loaded.m_areas);
}

int RunGeometryOnly(CountryConfig const & config, CountryPolicy const & policy)
{
  if (!Platform::MkDirChecked(FLAGS_out_dir))
  {
    LOG(LERROR, ("Cannot create out_dir", FLAGS_out_dir));
    return 3;
  }

  std::vector<m2::RegionD> helsinkiRegions;
  std::vector<m2::RegionD> const * helsinkiFilter = nullptr;
  if (!FLAGS_helsinki_poly.empty())
  {
    if (!LoadPolyFileAsMercatorRegions(FLAGS_helsinki_poly, helsinkiRegions))
    {
      LOG(LERROR, ("Failed to load Helsinki poly", FLAGS_helsinki_poly));
      return 4;
    }
    helsinkiFilter = &helsinkiRegions;
    LOG(LINFO, ("Loaded Helsinki poly regions:", helsinkiRegions.size()));
  }

  base::Timer timer;
  bool spotCheckOk = true;

  {
    std::vector<ExplorationArea> areas;
    auto const stats = FilterJsonlRings(FLAGS_rings, policy, /*centroidFilterMercator=*/nullptr, areas);
    SpaWriteParams params;
    params.m_mapDataVersion = FLAGS_map_data_version;
    params.m_policyVersion = config.GetPolicyVersion();
    params.m_isoCode = FLAGS_iso;
    params.m_mwmId = FLAGS_country_leaf;
    std::string const outPath = base::JoinPath(FLAGS_out_dir, FLAGS_country_leaf + SPA_FILE_EXTENSION);
    EmitGeometryOnly(outPath, std::move(areas), policy, params, stats, /*doSpotCheck=*/false);
  }

  if (helsinkiFilter)
  {
    std::vector<ExplorationArea> areas;
    auto const stats = FilterJsonlRings(FLAGS_rings, policy, helsinkiFilter, areas);
    SpaWriteParams params;
    params.m_mapDataVersion = FLAGS_map_data_version;
    params.m_policyVersion = config.GetPolicyVersion();
    params.m_isoCode = FLAGS_iso;
    params.m_mwmId = FLAGS_helsinki_leaf;
    std::string const outPath = base::JoinPath(FLAGS_out_dir, FLAGS_helsinki_leaf + SPA_FILE_EXTENSION);
    spotCheckOk = EmitGeometryOnly(outPath, std::move(areas), policy, params, stats, /*doSpotCheck=*/true);
  }
  else
  {
    LOG(LWARNING, ("No --helsinki_poly; skipped Helsinki leaf emit + known-id spot-check"));
  }

  std::cout << "elapsed_s=" << timer.ElapsedSeconds() << "\n";
  if (!spotCheckOk)
  {
    LOG(LERROR, ("Helsinki known-id spot-check failed (missing id or name mismatch)"));
    return 5;
  }
  return 0;
}

int RunProduction(CountryConfig const & config, CountryPolicy const & policy)
{
  if (FLAGS_borders_dir.empty() || FLAGS_pix_dir.empty())
  {
    LOG(LERROR, ("Production mode requires --borders_dir and --pix_dir"));
    return 1;
  }
  if (!Platform::MkDirChecked(FLAGS_out_dir))
  {
    LOG(LERROR, ("Cannot create out_dir", FLAGS_out_dir));
    return 3;
  }

  auto leaves = ListLeafBorders(FLAGS_borders_dir, FLAGS_border_prefix);
  if (leaves.empty())
  {
    LOG(LERROR, ("No leaf .poly borders found under", FLAGS_borders_dir, "prefix", FLAGS_border_prefix));
    return 4;
  }

  if (!FLAGS_leaf.empty())
  {
    std::vector<LeafBorder> filtered;
    for (auto const & leaf : leaves)
    {
      if (leaf.m_leafId == FLAGS_leaf)
        filtered.push_back(leaf);
    }
    if (filtered.empty())
    {
      LOG(LERROR, ("Requested --leaf not found among borders:", FLAGS_leaf));
      return 4;
    }
    leaves = std::move(filtered);
  }

  std::cout << "production leaves=" << leaves.size() << "\n";
  base::Timer timer;
  bool allOk = true;

  for (auto const & leaf : leaves)
  {
    std::vector<m2::RegionD> regions;
    if (!LoadPolyFileAsMercatorRegions(leaf.m_polyPath, regions))
    {
      LOG(LERROR, ("Failed to load leaf poly", leaf.m_polyPath));
      allOk = false;
      continue;
    }

    std::string const pixPath = base::JoinPath(FLAGS_pix_dir, leaf.m_leafId + PIX_FILE_EXTENSION);
    auto universe = ScanPixUniverseAscending(pixPath);
    if (!universe)
    {
      LOG(LERROR, ("Failed to scan ascending universe from", pixPath));
      allOk = false;
      continue;
    }
    if (universe->empty())
    {
      LOG(LERROR, ("Empty universe in", pixPath, "- production dense emit refuses empty U"));
      allOk = false;
      continue;
    }

    auto centres = MercatorCentresFromAscendingNest(*universe);
    if (!centres)
    {
      LOG(LERROR, ("Failed to build sample centres for", leaf.m_leafId));
      allOk = false;
      continue;
    }

    std::vector<ExplorationArea> areas;
    auto const stats = FilterJsonlRings(FLAGS_rings, policy, &regions, areas);
    SpaWriteParams params;
    params.m_mapDataVersion = FLAGS_map_data_version;
    params.m_policyVersion = config.GetPolicyVersion();
    params.m_isoCode = FLAGS_iso;
    params.m_mwmId = leaf.m_leafId;
    std::string const outPath = base::JoinPath(FLAGS_out_dir, leaf.m_leafId + SPA_FILE_EXTENSION);

    bool const isHelsinki = (leaf.m_leafId == FLAGS_helsinki_leaf);
    std::cout << "leaf=" << leaf.m_leafId << " |U|=" << universe->size() << " areas_admitted=" << stats.m_admitted
              << "\n";
    if (!EmitDenseLeaf(outPath, std::move(areas), *centres, policy, params, stats, /*doSpotCheck=*/isHelsinki,
                       FLAGS_verify_dense))
      allOk = false;
  }

  std::cout << "elapsed_s=" << timer.ElapsedSeconds() << "\n";
  return allOk ? 0 : 5;
}
}  // namespace

int main(int argc, char ** argv)
{
  gflags::SetUsageMessage(
      "Street Pixels .spa emit tool (SP-032 / SP-044).\n"
      "Default --mode=production: rings + policy + leaf borders + leaf .pix → dense\n"
      "{mwmLeafId}.spa (v2, AscendingNest, assign_count==|U|).\n"
      "--mode=geometry_only: fixtures/debug only (assign_count=0; not production).\n");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_rings.empty())
  {
    gflags::ShowUsageWithFlags(argv[0]);
    return 1;
  }

  std::string const policyPath = FLAGS_policy.empty() ? DefaultPolicyPath() : FLAGS_policy;
  auto const config = CountryConfig::LoadFromFile(policyPath);
  auto const & policy = config.GetByIso(FLAGS_iso);
  if (!policy.m_configured)
  {
    LOG(LERROR, ("ISO", FLAGS_iso, "is not configured in", policyPath));
    return 2;
  }

  if (FLAGS_mode == "production")
    return RunProduction(config, policy);
  if (FLAGS_mode == "geometry_only")
    return RunGeometryOnly(config, policy);

  LOG(LERROR, ("Unknown --mode", FLAGS_mode, "(expected production|geometry_only)"));
  return 1;
}
