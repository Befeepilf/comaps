#include "testing/testing.hpp"

#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_area_resolver.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/sparse_assignment_store.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/file_name_utils.hpp"

#include "defines.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
using namespace street_pixels;
using namespace street_pixels::test_helpers;

struct SpxFixture
{
  std::string m_spaPath;
  std::string m_spxPath;
  std::string m_pixPath;
  SpaWriteParams m_params;
  std::vector<ExplorationArea> m_areas;
  std::vector<m2::PointD> m_samples;
  std::vector<int64_t> m_universe;
  std::vector<int64_t> m_explored;
  std::vector<m2::PointD> m_exploredCentres;
  CountryPolicy m_policy;
  uint32_t m_policyVersion = 1;
};

SpxFixture MakeBaseFixture(std::string const & leaf, int64_t mapDataVersion, uint32_t policyVersion)
{
  auto configJson = std::string(R"({
  "policy_version": )") +
                    std::to_string(policyVersion) + R"(,
  "schema_version": 1,
  "countries": {
    "FI": {
      "mwm_root_ids": ["Finland"],
      "subdivision_admin_levels": [10, 9, 11],
      "settlement_admin_levels": [8],
      "place_boundaries": {
        "enabled": true,
        "place_types": ["neighbourhood", "quarter", "suburb"]
      }
    }
  }
})";
  auto const config = CountryConfig::LoadFromString(configJson);

  SpxFixture fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_policyVersion = config.GetPolicyVersion();
  fx.m_spaPath = ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.m_spxPath = SparseAssignmentPath(GetPlatform().WritableDir(), leaf);
  fx.m_pixPath = base::JoinPath(GetPlatform().WritableDir(), leaf + PIX_FILE_EXTENSION);
  fx.m_areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "District", LonLatBox(24.2, 60.2, 24.8, 60.8)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      fx.m_policy);
  fx.m_samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(24.1, 60.1),
      MercatorFromLonLat(30.0, 70.0),
  };
  fx.m_universe = {10, 20, 30};
  fx.m_explored = {10, 30};
  fx.m_exploredCentres = {fx.m_samples[0], fx.m_samples[2]};
  fx.m_params.m_mapDataVersion = mapDataVersion;
  fx.m_params.m_policyVersion = fx.m_policyVersion;
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = leaf;
  RemoveIfExists(fx.m_spaPath);
  RemoveIfExists(fx.m_spxPath);
  RemoveIfExists(fx.m_pixPath);
  WriteExplorationSidecar(fx.m_spaPath, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}

void CleanupFixture(SpxFixture const & fx)
{
  RemoveIfExists(fx.m_spaPath);
  RemoveIfExists(fx.m_spxPath);
  RemoveIfExists(fx.m_pixPath);
}

std::optional<ExplorationAreaResolver> LoadResolver(SpxFixture const & fx)
{
  return ExplorationAreaResolver::TryLoad(fx.m_spaPath, fx.m_universe, fx.m_params.m_mapDataVersion,
                                          fx.m_params.m_policyVersion);
}

uint64_t FileSizeOrZero(std::string const & path)
{
  uint64_t size = 0;
  if (!Platform::GetFileSizeByFullPath(path, size))
    return 0;
  return size;
}
}  // namespace

UNIT_TEST(SparseAssignment_PersistReloadRoundTrip)
{
  auto fx = MakeBaseFixture("sp030_roundtrip", 100, 1);
  auto resolver = LoadResolver(fx);
  TEST(resolver.has_value(), ());

  auto built = SparseAssignmentStore::Build(*resolver, fx.m_explored, fx.m_exploredCentres);
  TEST_EQUAL(built.Entries().size(), 2, ());
  TEST(built.Save(fx.m_spxPath), ());

  auto loaded = TryLoadAndVerifySparseAssignmentStore(fx.m_spxPath, 100, 1);
  TEST_EQUAL(loaded.m_status, SpxLoadStatus::Ok, (DebugPrint(loaded.m_status)));
  TEST_EQUAL(loaded.m_store.Entries().size(), built.Entries().size(), ());
  TEST_EQUAL(loaded.m_store.GetHeader().m_mapDataVersion, 100, ());
  TEST_EQUAL(loaded.m_store.GetHeader().m_policyVersion, 1, ());
  TEST_EQUAL(loaded.m_store.GetHeader().m_indexWidth, 2, ());

  for (size_t i = 0; i < built.Entries().size(); ++i)
  {
    TEST_EQUAL(loaded.m_store.Entries()[i].m_healpixNestId, built.Entries()[i].m_healpixNestId, ());
    TEST_EQUAL(loaded.m_store.Entries()[i].m_compactIndex, built.Entries()[i].m_compactIndex, ());
  }

  auto const * district = loaded.m_store.LookupArea(resolver->GetFile(), 10);
  TEST(district != nullptr, ());
  TEST_EQUAL(district->m_name, "District", ());
  TEST_EQUAL(loaded.m_store.LookupArea(resolver->GetFile(), 30), nullptr, ());

  CleanupFixture(fx);
}

UNIT_TEST(SparseAssignment_RematerializeMatchesResolver)
{
  auto fx = MakeBaseFixture("sp030_rematerialize", 200, 1);
  auto resolver = LoadResolver(fx);
  TEST(resolver.has_value(), ());

  auto rematerialized = SparseAssignmentStore::Rematerialize(*resolver, fx.m_explored, fx.m_exploredCentres);
  TEST_EQUAL(rematerialized.Entries().size(), fx.m_explored.size(), ());

  for (size_t i = 0; i < fx.m_explored.size(); ++i)
  {
    auto const * fromResolver = resolver->LookupByHealpix(fx.m_explored[i], fx.m_exploredCentres[i]);
    auto const compact = rematerialized.FindCompactIndex(fx.m_explored[i]);
    TEST(compact.has_value(), ());
    if (fromResolver == nullptr)
    {
      TEST_EQUAL(*compact, NoSubdivisionSentinel(rematerialized.GetHeader().m_indexWidth), ());
      TEST_EQUAL(rematerialized.LookupArea(resolver->GetFile(), fx.m_explored[i]), nullptr, ());
    }
    else
    {
      TEST_EQUAL(*compact, fromResolver->m_compactIndex, ());
      TEST_EQUAL(rematerialized.LookupArea(resolver->GetFile(), fx.m_explored[i]), fromResolver, ());
    }
  }

  CleanupFixture(fx);
}

UNIT_TEST(SparseAssignment_MapDataVersionBumpRebuilds)
{
  auto fx = MakeBaseFixture("sp030_map_bump", 10, 1);
  auto resolver = LoadResolver(fx);
  TEST(resolver.has_value(), ());
  auto store = SparseAssignmentStore::Build(*resolver, fx.m_explored, fx.m_exploredCentres);
  TEST(store.Save(fx.m_spxPath), ());

  fx.m_params.m_mapDataVersion = 11;
  RemoveIfExists(fx.m_spaPath);
  WriteExplorationSidecar(fx.m_spaPath, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);

  auto newResolver = ExplorationAreaResolver::TryLoad(fx.m_spaPath, fx.m_universe, 11, 1);
  TEST(newResolver.has_value(), ());

  auto verified = TryLoadAndVerifySparseAssignmentStore(fx.m_spxPath, 11, 1);
  TEST_EQUAL(verified.m_status, SpxLoadStatus::VersionMismatch, (DebugPrint(verified.m_status)));

  auto ensured = EnsureSparseAssignmentStore(fx.m_spxPath, *newResolver, fx.m_explored, fx.m_exploredCentres);
  TEST(ensured.has_value(), ());
  TEST_EQUAL(ensured->GetHeader().m_mapDataVersion, 11, ());
  TEST_EQUAL(ensured->GetHeader().m_policyVersion, 1, ());

  auto reloaded = TryLoadAndVerifySparseAssignmentStore(fx.m_spxPath, 11, 1);
  TEST_EQUAL(reloaded.m_status, SpxLoadStatus::Ok, ());

  CleanupFixture(fx);
}

UNIT_TEST(SparseAssignment_PolicyVersionBumpRebuilds)
{
  auto fx = MakeBaseFixture("sp030_policy_bump", 50, 1);
  auto resolver = LoadResolver(fx);
  TEST(resolver.has_value(), ());
  TEST(SparseAssignmentStore::Build(*resolver, fx.m_explored, fx.m_exploredCentres).Save(fx.m_spxPath), ());

  fx.m_params.m_policyVersion = 2;
  fx.m_policyVersion = 2;
  RemoveIfExists(fx.m_spaPath);
  WriteExplorationSidecar(fx.m_spaPath, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);

  auto newResolver = ExplorationAreaResolver::TryLoad(fx.m_spaPath, fx.m_universe, 50, 2);
  TEST(newResolver.has_value(), ());
  TEST_EQUAL(newResolver->GetFile().m_header.m_policyVersion, 2, ());

  auto verified = TryLoadAndVerifySparseAssignmentStore(fx.m_spxPath, 50, 2);
  TEST_EQUAL(verified.m_status, SpxLoadStatus::VersionMismatch, (DebugPrint(verified.m_status)));

  auto ensured = EnsureSparseAssignmentStore(fx.m_spxPath, *newResolver, fx.m_explored, fx.m_exploredCentres);
  TEST(ensured.has_value(), ());
  TEST_EQUAL(ensured->GetHeader().m_policyVersion, 2, ());
  TEST_EQUAL(ensured->GetHeader().m_mapDataVersion, 50, ());

  CleanupFixture(fx);
}

UNIT_TEST(SparseAssignment_CorruptRebuildsWithoutTouchingPix)
{
  auto fx = MakeBaseFixture("sp030_corrupt", 77, 1);
  auto resolver = LoadResolver(fx);
  TEST(resolver.has_value(), ());
  TEST(SparseAssignmentStore::Build(*resolver, fx.m_explored, fx.m_exploredCentres).Save(fx.m_spxPath), ());

  // Synthetic .pix marker — Ensure must never delete or rewrite it.
  {
    FileWriter writer(fx.m_pixPath, FileWriter::OP_WRITE_TRUNCATE);
    std::string const marker = "PIX_MUST_SURVIVE";
    writer.Write(marker.data(), marker.size());
  }
  auto const pixBefore = base::ReadFile(fx.m_pixPath);

  {
    FileWriter writer(fx.m_spxPath, FileWriter::OP_WRITE_TRUNCATE);
    uint8_t const garbage[] = {0xDE, 0xAD, 0xBE, 0xEF};
    writer.Write(garbage, sizeof(garbage));
  }

  auto corrupt = TryLoadSparseAssignmentStore(fx.m_spxPath);
  TEST_EQUAL(corrupt.m_status, SpxLoadStatus::Corrupt, (DebugPrint(corrupt.m_status)));

  auto ensured = EnsureSparseAssignmentStore(fx.m_spxPath, *resolver, fx.m_explored, fx.m_exploredCentres);
  TEST(ensured.has_value(), ());
  TEST_EQUAL(ensured->GetHeader().m_mapDataVersion, 77, ());

  auto const pixAfter = base::ReadFile(fx.m_pixPath);
  TEST_EQUAL(pixAfter, pixBefore, ());
  TEST_EQUAL(FileSizeOrZero(fx.m_pixPath), pixBefore.size(), ());

  auto reloaded = TryLoadAndVerifySparseAssignmentStore(fx.m_spxPath, 77, 1);
  TEST_EQUAL(reloaded.m_status, SpxLoadStatus::Ok, ());

  CleanupFixture(fx);
}

UNIT_TEST(SparseAssignment_MissingAreaIsNoneNoGrid)
{
  auto fx = MakeBaseFixture("sp030_missing_area", 88, 1);
  auto resolver = LoadResolver(fx);
  TEST(resolver.has_value(), ());

  // Rural explored cell → no subdivision, outside settlement → none.
  auto store = SparseAssignmentStore::Build(*resolver, std::vector<int64_t>{30},
                                            std::vector<m2::PointD>{fx.m_samples[2]});
  TEST_EQUAL(store.Entries().size(), 1, ());
  TEST_EQUAL(store.Entries()[0].m_compactIndex, NoSubdivisionSentinel(store.GetHeader().m_indexWidth), ());
  TEST_EQUAL(store.LookupArea(resolver->GetFile(), 30), nullptr, ());

  // Compact index pointing at a removed area row also yields none (no grid invent).
  SpxHeader header = store.GetHeader();
  std::vector<SparseAssignmentEntry> entries = store.Entries();
  entries[0].m_compactIndex = 999;
  SparseAssignmentStore bogus(header, entries);
  TEST_EQUAL(bogus.LookupArea(resolver->GetFile(), 30), nullptr, ());

  CleanupFixture(fx);
}

UNIT_TEST(SparseAssignment_PathBesidePixUsesSpxExtension)
{
  std::string const pix = base::JoinPath(GetPlatform().WritableDir(), "Finland_Uusimaa.pix");
  std::string const spx = SparseAssignmentPathBesidePix(pix);
  TEST_EQUAL(spx, base::JoinPath(GetPlatform().WritableDir(), std::string("Finland_Uusimaa") + SPX_FILE_EXTENSION),
             ());
}
