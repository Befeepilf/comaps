#include "testing/testing.hpp"

#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/areas_reader.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/sample_centres.hpp"
#include "street_pixels_areas/spa_jsonl.hpp"
#include "street_pixels_areas/subdivision_assigner.hpp"
#include "street_pixels_areas/subdivision_assignment.hpp"

#include "coding/file_writer.hpp"
#include "coding/writer.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace
{
using namespace street_pixels;
using namespace street_pixels::test_helpers;

// Minimal headered v2 `.pix` writer for synthetic universe fixtures (mirrors
// street_pixels_file::WriteHeader layout without linking map).
void WriteTinyPix(std::string const & path, std::vector<int64_t> const & ascendingNestIds)
{
  RemoveIfExists(path);
  FileWriter writer(path);
  uint32_t const magic = 0x58495053u;
  uint16_t const formatVersion = 2;
  uint16_t const flags = 1;
  int64_t const mapDataVersion = 260802;
  uint64_t const reserved = 0;
  WriteToSink(writer, magic);
  WriteToSink(writer, formatVersion);
  WriteToSink(writer, flags);
  WriteToSink(writer, mapDataVersion);
  WriteToSink(writer, reserved);
  for (int64_t id : ascendingNestIds)
    WriteToSink(writer, id);
}

std::vector<ExplorationArea> BuildFiAreas(CountryPolicy const & policy)
{
  std::vector<AreaCandidateInput> inputs = {
      MakeAdminCandidate(184714, 10, "Kamppi", LonLatBox(24.92, 60.16, 24.94, 60.17)),
      MakeAdminCandidate(34914, 8, "Helsinki", LonLatBox(24.80, 60.10, 25.10, 60.30)),
      MakePlaceCandidate(90, "neighbourhood", "Hood", LonLatBox(24.93, 60.165, 24.935, 60.168)),
  };
  std::vector<ExplorationArea> areas;
  uint32_t index = 0;
  for (auto const & input : inputs)
  {
    auto result = FilterExplorationCandidate(input, policy);
    TEST_EQUAL(result.m_reason, RejectReason::Accepted, (DebugPrint(result.m_reason)));
    result.m_area->m_compactIndex = index++;
    areas.push_back(*result.m_area);
  }
  return areas;
}
}  // namespace

UNIT_TEST(SampleCentres_RejectsScrambledNestIds)
{
  std::vector<int64_t> scrambled = {30, 10, 20};
  auto const centres = MercatorCentresFromAscendingNest(scrambled);
  TEST(!centres.has_value(), ());
}

UNIT_TEST(SampleCentres_AscendingRoundTripViaPix)
{
  int64_t const a = NestIdFromLonLat(24.93, 60.165);
  int64_t const b = NestIdFromLonLat(24.94, 60.166);
  int64_t const c = NestIdFromLonLat(24.95, 60.167);
  std::vector<int64_t> ids = {a, b, c};
  std::sort(ids.begin(), ids.end());
  // Ensure uniqueness after sort (distinct lon/lat should differ at nside).
  TEST_LESS(ids[0], ids[1], ());
  TEST_LESS(ids[1], ids[2], ());

  std::string const pixPath = base::JoinPath(GetPlatform().WritableDir(), "sp044_tiny.pix");
  WriteTinyPix(pixPath, ids);

  auto universe = ScanPixUniverseAscending(pixPath);
  TEST(universe.has_value(), ());
  TEST_EQUAL(*universe, ids, ());

  auto centres = MercatorCentresFromAscendingNest(*universe);
  TEST(centres.has_value(), ());
  TEST_EQUAL(centres->size(), 3u, ());

  RemoveIfExists(pixPath);
}

UNIT_TEST(SpaDenseEmit_ProductionRoundTrip)
{
  auto const config = FinlandConfig();
  auto const policy = config.GetByIso("FI");
  auto areas = BuildFiAreas(policy);

  int64_t const idInsideKamppi = NestIdFromLonLat(24.93, 60.165);
  int64_t const idInsideHelsinkiOnly = NestIdFromLonLat(24.85, 60.12);
  int64_t const idOutside = NestIdFromLonLat(30.0, 70.0);
  std::vector<int64_t> ids = {idInsideKamppi, idInsideHelsinkiOnly, idOutside};
  std::sort(ids.begin(), ids.end());

  auto centres = MercatorCentresFromAscendingNest(ids);
  TEST(centres.has_value(), ());

  std::string const spaPath = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp044_dense_tiny");
  RemoveIfExists(spaPath);

  SpaWriteParams params;
  params.m_mapDataVersion = 260802;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = "sp044_dense_tiny";
  WriteExplorationSidecar(spaPath, areas, *centres, policy, params);

  auto const loaded = ReadExplorationSidecar(spaPath);
  TEST_EQUAL(loaded.m_header.m_formatVersion, kSpaFormatVersion, ());
  TEST_EQUAL(loaded.m_header.m_nside, kSpaNside, ());
  TEST_EQUAL(loaded.m_header.m_universeOrder, kSpaUniverseOrderAscendingNest, ());
  TEST_EQUAL(loaded.m_header.m_assignCount, centres->size(), ());
  TEST_GREATER(loaded.m_header.m_assignCount, 0u, ());
  TEST_EQUAL(loaded.m_header.m_areaCount, 3u, ());

  // Settlement (Helsinki admin_8) must be present but never an assign target.
  bool sawSettlement = false;
  uint32_t settlementIndex = 0;
  for (auto const & area : loaded.m_areas)
  {
    if (area.m_role == AreaRole::Settlement)
    {
      sawSettlement = true;
      settlementIndex = area.m_compactIndex;
      TEST_EQUAL(area.m_osmId, 34914u, ());
      TEST(!area.IsAssignable(), ());
    }
  }
  TEST(sawSettlement, ());
  for (uint32_t idx : loaded.m_assignments)
  {
    TEST(idx != settlementIndex || idx == NoSubdivisionSentinel(loaded.m_header.m_indexWidth),
         ("settlement must not be an assign target", idx));
    if (idx != NoSubdivisionSentinel(loaded.m_header.m_indexWidth))
      TEST(loaded.m_areas[idx].IsAssignable(), (idx));
  }

  TEST(VerifyDenseAssignments(loaded, *centres, policy), ());

  // Geometry-only remains valid for fixtures but is not production default.
  std::string const geoPath = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp044_geo_only");
  RemoveIfExists(geoPath);
  WriteGeometryOnlyExplorationSidecar(geoPath, areas, policy, params);
  auto const geo = ReadExplorationSidecar(geoPath);
  TEST_EQUAL(geo.m_header.m_assignCount, 0u, ());
  TEST_EQUAL(geo.m_header.m_formatVersion, kSpaFormatVersion, ());

  RemoveIfExists(spaPath);
  RemoveIfExists(geoPath);
}

UNIT_TEST(SpaDenseEmit_AcceleratedMatchesPerPoint)
{
  auto const policy = FinlandPolicy();
  auto areas = BuildFiAreas(policy);
  for (uint32_t i = 0; i < areas.size(); ++i)
    areas[i].m_compactIndex = i;

  std::vector<m2::PointD> points = {
      MercatorFromLonLat(24.93, 60.165),
      MercatorFromLonLat(24.85, 60.12),
      MercatorFromLonLat(24.932, 60.166),
      MercatorFromLonLat(30.0, 70.0),
  };
  uint32_t const sentinel = kNoSubdivisionUint16;
  auto const dense = BuildDenseAssignments(points, areas, policy, sentinel);
  TEST_EQUAL(dense.size(), points.size(), ());
  for (size_t i = 0; i < points.size(); ++i)
    TEST_EQUAL(dense[i], AssignSubdivision(points[i], areas, policy, sentinel), (i));
}

UNIT_TEST(SpaDenseEmit_ListLeafBordersFinland)
{
  // Uses committed data/borders when ResourcesDir layout matches a desktop build;
  // otherwise skip via empty result when path missing.
  std::string const bordersDir = base::JoinPath(GetPlatform().ResourcesDir(), "..", "borders");
  auto leaves = ListLeafBorders(bordersDir, "Finland");
  if (leaves.empty())
  {
    // Fallback: workspace-relative path used by offline docs.
    leaves = ListLeafBorders("data/borders", "Finland");
  }
  if (leaves.empty())
  {
    LOG(LWARNING, ("No Finland borders found; skipping count assert"));
    return;
  }
  TEST_EQUAL(leaves.size(), 8u, (leaves.size()));
  bool sawHelsinki = false;
  for (auto const & leaf : leaves)
  {
    if (leaf.m_leafId == "Finland_Southern Finland_Helsinki")
      sawHelsinki = true;
  }
  TEST(sawHelsinki, ());
}
