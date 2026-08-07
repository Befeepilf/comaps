#include "street_pixels_areas/areas_reader.hpp"
#include "street_pixels_areas/spa_jsonl.hpp"

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

DEFINE_string(rings, "", "Path to SP-023 finland_admin_place_rings.jsonl");
DEFINE_string(policy, "", "Path to country_policies.json (default: ResourcesDir/street_pixels/...)");
DEFINE_string(iso, "FI", "ISO 3166-1 alpha-2 country code");
DEFINE_string(out_dir, "/tmp/sp032", "Output directory for .spa files (not committed)");
DEFINE_string(helsinki_poly, "", "Optional Helsinki MWM .poly for leaf centroid filter");
DEFINE_string(helsinki_leaf, "Finland_Southern Finland_Helsinki", "Helsinki MWM leaf id");
DEFINE_string(country_leaf, "Finland", "Country-concat measurement leaf id");
DEFINE_int64(map_data_version, 260802, "map_data_version stamped into .spa headers");

namespace
{
using namespace street_pixels;

std::string DefaultPolicyPath()
{
  return base::JoinPath(GetPlatform().ResourcesDir(), kCountryPoliciesRelativePath);
}

void PrintSizes(std::string const & label, SpaSectionSizes const & sizes, uint32_t areaCount)
{
  std::cout << label << " file_bytes=" << sizes.m_fileBytes << " (~" << (sizes.m_fileBytes / (1024.0 * 1024.0))
            << " MiB) hdr=" << sizes.m_hdrBytes << " areas=" << sizes.m_areasBytes
            << " assign=" << sizes.m_assignBytes << " area_count=" << areaCount << "\n";
}

void EmitOne(std::string const & outPath, std::vector<ExplorationArea> areas, CountryPolicy const & policy,
             SpaWriteParams const & params, JsonlFilterStats const & stats, bool doSpotCheck)
{
  WriteGeometryOnlyExplorationSidecar(outPath, areas, policy, params);
  auto const loaded = ReadExplorationSidecar(outPath);
  auto const sizes = MeasureSpaSectionSizes(outPath);

  std::cout << "wrote " << outPath << "\n";
  std::cout << "  filter records=" << stats.m_records << " admitted=" << stats.m_admitted;
  for (auto const & [reason, count] : stats.m_rejects)
    std::cout << " " << reason << "=" << count;
  std::cout << "\n";
  std::cout << "  header area_count=" << loaded.m_header.m_areaCount
            << " assign_count=" << loaded.m_header.m_assignCount
            << " policy_version=" << loaded.m_header.m_policyVersion
            << " map_data_version=" << loaded.m_header.m_mapDataVersion
            << " mwm_id=" << loaded.m_header.m_mwmId << "\n";
  PrintSizes("  sections", sizes, loaded.m_header.m_areaCount);

  if (doSpotCheck)
  {
    auto const checks = SpotCheckKnownIds(loaded.m_areas, HelsinkiKnownOsmIds());
    uint32_t found = 0;
    std::cout << "  helsinki_known_id_spot_check:\n";
    for (auto const & row : checks)
    {
      if (row.m_found)
        ++found;
      std::cout << "    osm_id=" << row.m_osmId << " hint=" << row.m_expectedNameHint
                << " found=" << (row.m_found ? "yes" : "no");
      if (row.m_found)
        std::cout << " name=" << row.m_actualName << " role=" << DebugPrint(row.m_role)
                  << " admin_level=" << static_cast<int>(row.m_adminLevel);
      std::cout << "\n";
    }
    std::cout << "  known_ids_found=" << found << "/" << checks.size() << "\n";
  }
}
}  // namespace

int main(int argc, char ** argv)
{
  gflags::SetUsageMessage(
      "Offline Street Pixels .spa emit harness (SP-032).\n"
      "Reads SP-023 JSONL rings + country policy, filters, writes geometry-only\n"
      "Helsinki leaf + FI country-concat .spa under --out_dir (not committed).\n");
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

  // Country-concat: all admitted FI rings (size vs SP-023 national baseline).
  {
    std::vector<ExplorationArea> areas;
    auto const stats = FilterJsonlRings(FLAGS_rings, policy, /*centroidFilterMercator=*/nullptr, areas);
    SpaWriteParams params;
    params.m_mapDataVersion = FLAGS_map_data_version;
    params.m_policyVersion = config.GetPolicyVersion();
    params.m_isoCode = FLAGS_iso;
    params.m_mwmId = FLAGS_country_leaf;
    std::string const outPath = base::JoinPath(FLAGS_out_dir, FLAGS_country_leaf + SPA_FILE_EXTENSION);
    EmitOne(outPath, std::move(areas), policy, params, stats, /*doSpotCheck=*/false);
  }

  // Helsinki leaf: centroid-in-border attribution (matches SP-023 measure_sizes).
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
    EmitOne(outPath, std::move(areas), policy, params, stats, /*doSpotCheck=*/true);
  }
  else
  {
    LOG(LWARNING, ("No --helsinki_poly; skipped Helsinki leaf emit + known-id spot-check"));
  }

  std::cout << "elapsed_s=" << timer.ElapsedSeconds() << "\n";
  return 0;
}
