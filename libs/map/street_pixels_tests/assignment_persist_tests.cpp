#include "testing/testing.hpp"

#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"

#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_area_resolver.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/sparse_assignment_store.hpp"

#include "street_pixels_config/country_config.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include "base/file_name_utils.hpp"

#include "geometry/mercator.hpp"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace
{
std::string PersistPath(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void PersistRemove(std::string const & path) { Platform::RemoveFileIfExists(path); }

std::vector<m2::PointD> LonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput MakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
                                            std::vector<m2::PointD> const & ring)
{
  street_pixels::AreaCandidateInput input;
  input.m_osmId = osmId;
  input.m_osmType = street_pixels::OsmObjectType::Relation;
  input.m_geometrySource = street_pixels::GeometrySource::TrueClosedRing;
  input.m_name = name;
  input.m_kind = "admin";
  input.m_adminLevel = adminLevel;
  input.m_lonLatRings = {ring};
  return input;
}
}  // namespace

UNIT_TEST(AssignmentPersist_CleanupDeletesSpx)
{
  std::string const countryId = "sp030_cleanup_spx";
  std::string const pixPath = PersistPath(countryId + ".pix");
  std::string const spxPath = street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), countryId);
  PersistRemove(pixPath);
  PersistRemove(spxPath);
  PersistRemove(PersistPath(countryId + ".pixr"));

  TEST(street_pixels_file::SaveUnexploredIds(pixPath, {11, 22}, 3), ());
  {
    FileWriter writer(spxPath, FileWriter::OP_WRITE_TRUNCATE);
    char const marker[] = "spx";
    writer.Write(marker, sizeof(marker) - 1);
  }
  TEST(Platform::IsFileExistsByFullPath(spxPath), ());

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.CleanupStreetPixelsForTesting(countryId);

  TEST(!Platform::IsFileExistsByFullPath(spxPath), ());
  PersistRemove(PersistPath(countryId + ".pixr"));
}

UNIT_TEST(AssignmentPersist_ScanUniverseAscending)
{
  std::string const path = PersistPath("sp030_universe.pix");
  PersistRemove(path);
  TEST(street_pixels_file::SaveRematchedUniverse(
           path, {10, 20, 30},
           street_pixels_file::ExploredEverLiveMap{{10, true}, {30, false}}, 5),
       ());

  auto const universe = street_pixels_file::ScanUniverseAscending(path);
  TEST(universe.has_value(), ());
  TEST_EQUAL(*universe, std::vector<int64_t>({10, 20, 30}), ());

  auto const explored = street_pixels_file::ScanExploredEverLive(path);
  TEST(explored.has_value(), ());
  TEST_EQUAL(explored->size(), 2, ());

  PersistRemove(path);
}

UNIT_TEST(AssignmentPersist_PolicyBumpRematerializeKeepsPix)
{
  std::string const leaf = "sp030_policy_mgr";
  std::string const spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  std::string const pixPath = PersistPath(leaf + ".pix");
  std::string const spxPath = street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), leaf);
  PersistRemove(spaPath);
  PersistRemove(pixPath);
  PersistRemove(spxPath);

  auto const config = street_pixels::CountryConfig::LoadFromString(R"({
  "policy_version": 1,
  "schema_version": 1,
  "countries": {
    "FI": {
      "mwm_root_ids": ["Finland"],
      "subdivision_admin_levels": [10, 9, 11],
      "settlement_admin_levels": [8],
      "place_boundaries": { "enabled": true, "place_types": ["neighbourhood"] }
    }
  }
})");
  auto const policy = config.GetByIso("FI");

  std::vector<street_pixels::ExplorationArea> areas;
  for (auto const & input : {MakeAdmin(10, 10, "District", LonLatBox(24.2, 60.2, 24.8, 60.8)),
                             MakeAdmin(8, 8, "City", LonLatBox(24.0, 60.0, 25.0, 61.0))})
  {
    auto result = street_pixels::FilterExplorationCandidate(input, policy);
    TEST(result.m_area.has_value(), ());
    areas.push_back(*result.m_area);
  }

  std::vector<m2::PointD> samples = {
      mercator::FromLatLon(60.5, 24.5),
      mercator::FromLatLon(60.1, 24.1),
  };
  street_pixels::SpaWriteParams params;
  params.m_mapDataVersion = 42;
  params.m_policyVersion = 1;
  params.m_isoCode = "FI";
  params.m_mwmId = leaf;
  street_pixels::WriteExplorationSidecar(spaPath, areas, samples, policy, params);

  street_pixels_file::ExploredEverLiveMap seed{{10, true}};
  TEST(street_pixels_file::SaveRematchedUniverse(pixPath, {10, 20}, seed, 42), ());

  // Seed an old .spx with policy 1 via Ensure.
  {
    auto universe = street_pixels_file::ScanUniverseAscending(pixPath);
    TEST(universe.has_value(), ());
    auto resolver =
        street_pixels::ExplorationAreaResolver::TryLoad(spaPath, *universe, 42, 1);
    TEST(resolver.has_value(), ());
    auto ensured = street_pixels::EnsureSparseAssignmentStore(
        spxPath, *resolver, std::vector<int64_t>{10},
        std::vector<m2::PointD>{samples[0]});
    TEST(ensured.has_value(), ());
  }

  // Rewrite sidecar at policy 2 (same map-data).
  params.m_policyVersion = 2;
  PersistRemove(spaPath);
  street_pixels::WriteExplorationSidecar(spaPath, areas, samples, policy, params);

  auto const pixBefore = [&]()
  {
    FileReader reader(pixPath);
    std::vector<uint8_t> bytes(static_cast<size_t>(reader.Size()));
    reader.Read(0, bytes.data(), bytes.size());
    return bytes;
  }();

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RematerializeAssignmentsOnPolicyBump(leaf, spaPath, 42, 2), ());

  auto const pixAfter = [&]()
  {
    FileReader reader(pixPath);
    std::vector<uint8_t> bytes(static_cast<size_t>(reader.Size()));
    reader.Read(0, bytes.data(), bytes.size());
    return bytes;
  }();
  TEST_EQUAL(pixAfter, pixBefore, ());

  auto loaded = street_pixels::TryLoadAndVerifySparseAssignmentStore(spxPath, 42, 2);
  TEST_EQUAL(loaded.m_status, street_pixels::SpxLoadStatus::Ok, ());

  auto signal = manager.TakePendingAssignmentRematch(leaf);
  TEST(signal.has_value(), ());
  TEST(signal->policyOnly, ());
  TEST_EQUAL(signal->policyVersion, 2, ());

  PersistRemove(spaPath);
  PersistRemove(pixPath);
  PersistRemove(spxPath);
}
