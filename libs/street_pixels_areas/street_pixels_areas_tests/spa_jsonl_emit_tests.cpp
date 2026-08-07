#include "testing/testing.hpp"

#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/areas_reader.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/spa_jsonl.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include "defines.hpp"

#include <fstream>
#include <string>

namespace
{
using namespace street_pixels;
using namespace street_pixels::test_helpers;

std::string WriteTinyJsonl()
{
  std::string const path = base::JoinPath(GetPlatform().WritableDir(), "sp032_tiny_rings.jsonl");
  RemoveIfExists(path);
  std::ofstream out(path);
  // Two closed admin rings + one unnamed reject + one policy_mismatch admin_7.
  out << R"({"osm_type":"relation","osm_id":184714,"name":"Kamppi","kind":"admin","admin_level":10,"place":"","class_key":"admin_10","rings":[[[24.92,60.16],[24.94,60.16],[24.94,60.17],[24.92,60.17],[24.92,60.16]]],"centroid":[24.93,60.165]})"
      << "\n";
  out << R"({"osm_type":"relation","osm_id":34914,"name":"Helsinki","kind":"admin","admin_level":8,"place":"city","class_key":"admin_8","rings":[[[24.80,60.10],[25.10,60.10],[25.10,60.30],[24.80,60.30],[24.80,60.10]]],"centroid":[24.95,60.20]})"
      << "\n";
  out << R"({"osm_type":"relation","osm_id":1,"name":"","kind":"admin","admin_level":10,"place":"","class_key":"admin_10","rings":[[[24.0,60.0],[24.1,60.0],[24.1,60.1],[24.0,60.1],[24.0,60.0]]],"centroid":[24.05,60.05]})"
      << "\n";
  out << R"({"osm_type":"relation","osm_id":2,"name":"WrongLevel","kind":"admin","admin_level":7,"place":"","class_key":"admin_7","rings":[[[24.0,60.0],[24.1,60.0],[24.1,60.1],[24.0,60.1],[24.0,60.0]]],"centroid":[24.05,60.05]})"
      << "\n";
  out << R"({"osm_type":"relation","osm_id":90,"name":"Hood","kind":"place","admin_level":null,"place":"neighbourhood","class_key":"place_neighbourhood","rings":[[[24.93,60.165],[24.935,60.165],[24.935,60.168],[24.93,60.168],[24.93,60.165]]],"centroid":[24.932,60.166]})"
      << "\n";
  return path;
}
}  // namespace

UNIT_TEST(SpaJsonlEmit_TinyRoundTrip)
{
  auto const config = FinlandConfig();
  auto const policy = config.GetByIso("FI");
  std::string const jsonl = WriteTinyJsonl();

  std::vector<ExplorationArea> areas;
  auto const stats = FilterJsonlRings(jsonl, policy, /*centroidFilterMercator=*/nullptr, areas);
  TEST_EQUAL(stats.m_admitted, 3u, ());
  TEST_EQUAL(stats.m_rejects.at("Unnamed"), 1u, ());
  TEST_EQUAL(stats.m_rejects.at("PolicyMismatch"), 1u, ());

  std::string const spaPath =
      ExplorationSidecarPath(GetPlatform().WritableDir(), "sp032_tiny_emit");
  RemoveIfExists(spaPath);

  SpaWriteParams params;
  params.m_mapDataVersion = 260802;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = "sp032_tiny_emit";
  WriteGeometryOnlyExplorationSidecar(spaPath, areas, policy, params);

  auto const loaded = ReadExplorationSidecar(spaPath);
  TEST_EQUAL(loaded.m_header.m_areaCount, 3u, ());
  TEST_EQUAL(loaded.m_header.m_assignCount, 0u, ());
  TEST_EQUAL(loaded.m_header.m_magic, kSpaMagic, ());
  TEST_EQUAL(loaded.m_areas.size(), 3u, ());

  auto const sizes = MeasureSpaSectionSizes(spaPath);
  TEST_GREATER(sizes.m_fileBytes, 0u, ());
  TEST_GREATER(sizes.m_areasBytes, 0u, ());
  TEST_EQUAL(sizes.m_assignBytes, 0u, ());

  bool sawKamppi = false;
  bool sawHelsinki = false;
  bool sawHood = false;
  for (auto const & area : loaded.m_areas)
  {
    if (area.m_osmId == 184714)
    {
      sawKamppi = true;
      TEST_EQUAL(area.m_name, "Kamppi", ());
      TEST_EQUAL(area.m_role, AreaRole::Subdivision, ());
    }
    if (area.m_osmId == 34914)
    {
      sawHelsinki = true;
      TEST_EQUAL(area.m_role, AreaRole::Settlement, ());
    }
    if (area.m_osmId == 90)
    {
      sawHood = true;
      TEST_EQUAL(area.m_role, AreaRole::PlaceBoundary, ());
    }
  }
  TEST(sawKamppi && sawHelsinki && sawHood, ());

  auto const checks = SpotCheckKnownIds(loaded.m_areas, HelsinkiKnownOsmIds());
  uint32_t found = 0;
  uint32_t nameOk = 0;
  for (auto const & row : checks)
  {
    if (row.m_found)
      ++found;
    if (row.m_nameMatches)
      ++nameOk;
    if (row.m_osmId == 184714 || row.m_osmId == 34914)
    {
      TEST(row.m_found, (row.m_osmId));
      TEST(row.m_nameMatches, (row.m_expectedNameHint, row.m_actualName));
    }
  }
  TEST_EQUAL(found, 2u, ());  // Kamppi + Helsinki in the tiny fixture
  TEST_EQUAL(nameOk, 2u, ());

  RemoveIfExists(spaPath);
  RemoveIfExists(jsonl);
}

UNIT_TEST(SpaJsonlEmit_ParseNullAdminLevel)
{
  auto const line =
      R"({"osm_type":"relation","osm_id":3,"name":"N","kind":"place","admin_level":null,"place":"suburb","rings":[[[24.0,60.0],[24.1,60.0],[24.1,60.1],[24.0,60.1],[24.0,60.0]]],"centroid":[24.05,60.05]})";
  auto const parsed = ParseJsonlRingLine(line);
  TEST(parsed.has_value(), ());
  TEST_EQUAL(parsed->m_input.m_adminLevel, -1, ());
  TEST_EQUAL(parsed->m_input.m_placeType, "suburb", ());
  TEST(parsed->m_centroidLonLat.has_value(), ());
}
