#include "testing/testing.hpp"

#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_pix_derive.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "street_pixels_areas/sample_centres.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_header.hpp"
#include "indexer/data_source.hpp"
#include "indexer/features_vector.hpp"

#include "geometry/mercator.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"
#include "coding/internal/file_data.hpp"

#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include "defines.hpp"

#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string PixDeriveFixtureMwmPath()
{
  std::string const writable = base::JoinPath(GetPlatform().WritableDir(), "minsk-pass" DATA_FILE_EXTENSION);
  if (Platform::IsFileExistsByFullPath(writable))
    return writable;
  return base::JoinPath(GetPlatform().ResourcesDir(), "minsk-pass" DATA_FILE_EXTENSION);
}

std::string PixDeriveTestOutDir()
{
  return base::JoinPath(GetPlatform().WritableDir(), "sp099_pix_derive");
}

void PixDeriveRemoveIfExists(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
}

std::vector<uint8_t> PixDeriveReadAllBytes(std::string const & path)
{
  FileReader reader(path);
  std::vector<uint8_t> bytes(static_cast<size_t>(reader.Size()));
  if (!bytes.empty())
    reader.Read(0, bytes.data(), bytes.size());
  return bytes;
}

void PixDeriveRequireFixtureMwm(std::string const & path)
{
  TEST(Platform::IsFileExistsByFullPath(path), ("minsk-pass.mwm must be present for SP-099 derive tests", path));
}
}  // namespace

UNIT_TEST(PixDerive_SegmentizeStreetUses15mSampling)
{
  TEST_ALMOST_EQUAL_ABS(kPathSamplingStepMeters, 15.0, 1e-12, ());

  double const lat = 48.2;
  double const lon = 16.37;
  m2::PointD const from = mercator::FromLatLon(lat, lon);

  auto const offsetNorth = [&](double northM)
  {
    auto const [endLat, endLon] = street_pixels_tests::OffsetLatLonByMeters(lat, lon, northM, 0.0);
    m2::PointD const to = mercator::FromLatLon(endLat, endLon);
    return std::make_pair(mercator::DistanceOnEarth(from, to), to);
  };

  {
    auto const [endLat, endLon] = street_pixels_tests::OffsetLatLonByMeters(lat, lon, 14.0, 0.0);
    m2::PointD const to = mercator::FromLatLon(endLat, endLon);
    TEST_LESS(mercator::DistanceOnEarth(from, to), kPathSamplingStepMeters, ());
    size_t count = 0;
    SegmentizeStreet(from, to, [&](m2::PointD const &, double) { ++count; });
    TEST_EQUAL(count, 0, ());
  }

  {
    auto const [d, to] = offsetNorth(14.98);
    TEST_LESS_OR_EQUAL(d, kPathSamplingStepMeters, ());
    TEST_GREATER(d, kPathSamplingStepMeters - 0.05, ());
    size_t count = 0;
    SegmentizeStreet(from, to, [&](m2::PointD const &, double) { ++count; });
    TEST_EQUAL(count, 0, ());
  }

  {
    auto const [d, to] = offsetNorth(15.02);
    TEST_GREATER(d, kPathSamplingStepMeters, ());
    TEST_LESS(d, kPathSamplingStepMeters + 0.05, ());
    size_t count = 0;
    SegmentizeStreet(from, to, [&](m2::PointD const &, double) { ++count; });
    TEST_EQUAL(count, 1, ());
  }

  {
    auto const [endLat, endLon] = street_pixels_tests::OffsetLatLonByMeters(lat, lon, 16.0, 0.0);
    m2::PointD const to = mercator::FromLatLon(endLat, endLon);
    TEST_GREATER(mercator::DistanceOnEarth(from, to), kPathSamplingStepMeters, ());
    size_t count = 0;
    SegmentizeStreet(from, to, [&](m2::PointD const &, double) { ++count; });
    TEST_EQUAL(count, 1, ());
  }
}

UNIT_TEST(PixDerive_EmptyOutDirIsBadOutput)
{
  auto const missingOut = street_pixels::DeriveAndWritePixFile("leaf.mwm", "", 0);
  TEST_EQUAL(static_cast<int>(missingOut.m_status), static_cast<int>(street_pixels::PixDeriveStatus::BadOutput), ());
  TEST_EQUAL(street_pixels::PixDeriveStatusExitCode(missingOut.m_status), 5, ());
}

UNIT_TEST(PixDerive_WriteUnexploredUniversePixFailClosedOnEmpty)
{
  std::string const outDir = PixDeriveTestOutDir();
  TEST(Platform::MkDirChecked(outDir), ());
  std::string const path = base::JoinPath(outDir, "empty.pix");
  PixDeriveRemoveIfExists(path);

  std::set<std::int64_t> empty;
  auto const status = street_pixels::WriteUnexploredUniversePix(path, empty, 260829);
  TEST_EQUAL(static_cast<int>(status), static_cast<int>(street_pixels::PixDeriveStatus::EmptyUniverse), ());
  TEST(!Platform::IsFileExistsByFullPath(path), ());
}

UNIT_TEST(PixDerive_FailClosedMissingAndCorruptMwm)
{
  classificator::Load();
  std::string const outDir = PixDeriveTestOutDir();
  TEST(Platform::MkDirChecked(outDir), ());

  {
    auto const missing = street_pixels::DeriveAndWritePixFile(
        base::JoinPath(outDir, "no-such-leaf.mwm"), outDir, 0);
    TEST_EQUAL(static_cast<int>(missing.m_status), static_cast<int>(street_pixels::PixDeriveStatus::MissingMwm), ());
    TEST_EQUAL(street_pixels::PixDeriveStatusExitCode(missing.m_status), 1, ());
    TEST(!Platform::IsFileExistsByFullPath(base::JoinPath(outDir, "no-such-leaf.pix")), ());
  }

  {
    std::string const corruptPath = base::JoinPath(outDir, "corrupt-leaf.mwm");
    PixDeriveRemoveIfExists(corruptPath);
    {
      FileWriter writer(corruptPath, FileWriter::OP_WRITE_TRUNCATE);
      char const garbage[] = "not an mwm";
      writer.Write(garbage, sizeof(garbage) - 1);
      writer.Flush();
    }
    auto const corrupt = street_pixels::DeriveAndWritePixFile(corruptPath, outDir, 0);
    TEST_EQUAL(static_cast<int>(corrupt.m_status), static_cast<int>(street_pixels::PixDeriveStatus::UnreadableMwm), ());
    TEST_EQUAL(street_pixels::PixDeriveStatusExitCode(corrupt.m_status), 2, ());
    TEST(!Platform::IsFileExistsByFullPath(base::JoinPath(outDir, "corrupt-leaf.pix")), ());
    PixDeriveRemoveIfExists(corruptPath);
  }

  {
    auto const emptyOut = street_pixels::DeriveAndWritePixFile("", outDir, 0);
    TEST_EQUAL(static_cast<int>(emptyOut.m_status), static_cast<int>(street_pixels::PixDeriveStatus::BadOutput), ());
  }
}

UNIT_TEST(PixDerive_UniverseRoundTripOnFixtureMwm)
{
  classificator::Load();
  std::string const mwmPath = PixDeriveFixtureMwmPath();
  PixDeriveRequireFixtureMwm(mwmPath);

  std::string const outDir = PixDeriveTestOutDir();
  TEST(Platform::MkDirChecked(outDir), ());
  std::string const pixPath = base::JoinPath(outDir, "minsk-pass.pix");
  PixDeriveRemoveIfExists(pixPath);

  FeaturesVectorTest featuresVector(mwmPath);
  TEST_EQUAL(static_cast<int>(featuresVector.GetHeader().GetType()),
             static_cast<int>(feature::DataHeader::MapType::Country), ());
  auto const universe = DeriveStreetPixelsUniverse(featuresVector);
  TEST(!universe.empty(), ());

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  FeaturesVectorTest featuresVectorForManager(mwmPath);
  auto const managerEmptyCountry = manager.DeriveStreetPixelsFromFeatures(featuresVectorForManager);
  TEST(managerEmptyCountry.empty(), ());

  manager.SetCountryIdForTesting("minsk-pass");
  auto const managerUniverse = manager.DeriveStreetPixelsFromFeatures(featuresVectorForManager);
  TEST_EQUAL(managerUniverse, universe, ());

  int64_t const versionFromMwm =
      static_cast<int64_t>(version::MwmVersion::Read(FilesContainerR(mwmPath)).GetVersion());
  TEST_GREATER(versionFromMwm, 0, ());

  auto const derived = street_pixels::DeriveAndWritePixFile(mwmPath, outDir, 0);
  TEST_EQUAL(static_cast<int>(derived.m_status), static_cast<int>(street_pixels::PixDeriveStatus::Ok), ());
  TEST_EQUAL(derived.m_leafId, std::string("minsk-pass"), ());
  TEST_EQUAL(derived.m_outPath, pixPath, ());
  TEST_EQUAL(derived.m_universeSize, universe.size(), ());
  TEST_EQUAL(derived.m_mapDataVersion, versionFromMwm, ());

  auto const scanned = street_pixels_file::ScanUniverseAscending(pixPath);
  TEST(scanned.has_value(), ());
  TEST_EQUAL(scanned->size(), universe.size(), ());
  TEST_EQUAL(std::set<std::int64_t>(scanned->begin(), scanned->end()), universe, ());
  for (size_t i = 1; i < scanned->size(); ++i)
    TEST_LESS((*scanned)[i - 1], (*scanned)[i], ());

  auto const explored = street_pixels_file::ScanExploredEverLive(pixPath);
  TEST(explored.has_value(), ());
  TEST_EQUAL(explored->size(), 0, ());

  auto const probe = street_pixels_file::ProbeFile(pixPath);
  TEST_EQUAL(static_cast<int>(probe.kind), static_cast<int>(street_pixels_file::FileKind::HeaderedV2), ());
  TEST_EQUAL(probe.header.mapDataVersion, versionFromMwm, ());

  auto const areasScan = street_pixels::ScanPixUniverseAscending(pixPath);
  TEST(areasScan.has_value(), ());
  TEST_EQUAL(*areasScan, *scanned, ());

  auto const firstBytes = PixDeriveReadAllBytes(pixPath);
  auto const again = street_pixels::DeriveAndWritePixFile(mwmPath, outDir, 0);
  TEST_EQUAL(static_cast<int>(again.m_status), static_cast<int>(street_pixels::PixDeriveStatus::Ok), ());
  auto const secondBytes = PixDeriveReadAllBytes(pixPath);
  TEST_EQUAL(firstBytes, secondBytes, ());

  auto const overridden = street_pixels::DeriveAndWritePixFile(mwmPath, outDir, 260829);
  TEST_EQUAL(static_cast<int>(overridden.m_status), static_cast<int>(street_pixels::PixDeriveStatus::Ok), ());
  TEST_EQUAL(overridden.m_mapDataVersion, 260829, ());
  auto const overrideProbe = street_pixels_file::ProbeFile(pixPath);
  TEST_EQUAL(overrideProbe.header.mapDataVersion, 260829, ());

  PixDeriveRemoveIfExists(pixPath);
}

UNIT_TEST(PixDerive_FailClosedOnWorldAndWorldCoastsFilenames)
{
  classificator::Load();
  std::string const mwmPath = PixDeriveFixtureMwmPath();
  PixDeriveRequireFixtureMwm(mwmPath);

  std::string const outDir = PixDeriveTestOutDir();
  TEST(Platform::MkDirChecked(outDir), ());

  auto const refuseNamedCopy = [&](char const * fileName)
  {
    std::string const namedPath = base::JoinPath(outDir, fileName);
    std::string const pixPath = base::JoinPath(outDir, base::FilenameWithoutExt(fileName) + PIX_FILE_EXTENSION);
    PixDeriveRemoveIfExists(namedPath);
    PixDeriveRemoveIfExists(pixPath);
    TEST(base::CopyFileX(mwmPath, namedPath), (mwmPath, namedPath));

    auto const result = street_pixels::DeriveAndWritePixFile(namedPath, outDir, 0);
    TEST_EQUAL(static_cast<int>(result.m_status), static_cast<int>(street_pixels::PixDeriveStatus::NotALeaf),
               (fileName));
    TEST_EQUAL(street_pixels::PixDeriveStatusExitCode(result.m_status), 1, ());
    TEST(!Platform::IsFileExistsByFullPath(pixPath), (pixPath));

    PixDeriveRemoveIfExists(namedPath);
    PixDeriveRemoveIfExists(pixPath);
  };

  refuseNamedCopy(WORLD_FILE_NAME DATA_FILE_EXTENSION);
  refuseNamedCopy(WORLD_COASTS_FILE_NAME DATA_FILE_EXTENSION);
}
