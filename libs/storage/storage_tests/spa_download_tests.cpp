#include "testing/testing.hpp"

#include "storage/storage.hpp"
#include "storage/downloading_policy.hpp"
#include "storage/storage_tests/fake_map_files_downloader.hpp"
#include "storage/storage_tests/task_runner.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "base/string_utils.hpp"

#include "defines.hpp"

#include <memory>
#include <string>

namespace storage
{
namespace
{
using platform::CountryFile;
using platform::LocalCountryFile;

std::string const kMapTestDir = "spa-download-tests";

class SpaDownloadDenyPolicy : public DownloadingPolicy
{
public:
  bool IsDownloadingAllowed() override { return false; }
};

std::string MakeSpaDownloadCountriesJson(std::string const & leafFields)
{
  return std::string(R"({
    "id": "Countries",
    "v": )") +
         strings::to_string(version::FOR_TESTING_MWM1) + R"(,
    "g": [
      {
        "id": "SpaLeaf")" +
         leafFields + R"(
      }
    ]
  })";
}

void InitSpaStorage(Storage & storage, TaskRunner & runner)
{
  storage.Init([](CountryId const &, LocalFilePtr const) {},
               [](CountryId const &, LocalFilePtr const) { return false; });
  storage.SetDownloaderForTesting(std::make_unique<FakeMapFilesDownloader>(runner));
  storage.SetCurrentDataVersionForTesting(version::FOR_TESTING_MWM1);
  storage.SetEnabledIntegrityValidationForTesting(false);
}

void DownloadAndPump(Storage & storage, TaskRunner & runner, CountryId const & id, MapFileType type)
{
  storage.DownloadCountry(id, type);
  // Fake downloader parks requests in pending until countries-check completes; kick them.
  storage.StartPendingDownloadsForTesting();
  runner.Run();
}

UNIT_TEST(Storage_SpaDownload_AdvertisedMapThenSpa)
{
  WritableDirChanger const writableDirChanger(kMapTestDir);
  Platform::ThreadRunner threadRunner;

  auto const json = MakeSpaDownloadCountriesJson(R"(,
        "s": 2048,
        "sha1_base64": "mwmSha",
        "spa": 512,
        "spa_sha1_base64": "spaSha")");

  TaskRunner runner;
  Storage storage(json, std::make_unique<FakeMapFilesDownloader>(runner));
  InitSpaStorage(storage, runner);

  CountryId const id = "SpaLeaf";
  TEST(storage.GetCountryFile(id).HasRemoteSpa(), ());
  TEST_EQUAL(2048 + 512, storage.CountrySizeInBytes(id).second, ());
  TEST_EQUAL(2048 + 512, storage.CountryLeafByCountryId(id).GetSubtreeMwmSizeBytes(), ());

  storage.DeleteCountry(id, MapFileType::Map);
  TEST_EQUAL(Status::NotDownloaded, storage.CountryStatusEx(id), ());

  DownloadAndPump(storage, runner, id, MapFileType::Map);

  auto localFile = storage.GetLatestLocalFile(id);
  TEST(localFile, ());
  localFile->SyncWithDisk();
  TEST(localFile->OnDisk(MapFileType::Map), ());
  TEST(localFile->OnDisk(MapFileType::Spa), ());
  TEST_EQUAL(Status::OnDisk, storage.CountryStatusEx(id), ());
}

UNIT_TEST(Storage_SpaDownload_NoAdvertiseNeverQueuesSpa)
{
  WritableDirChanger const writableDirChanger(kMapTestDir);
  Platform::ThreadRunner threadRunner;

  auto const json = MakeSpaDownloadCountriesJson(R"(,
        "s": 1024,
        "sha1_base64": "mwmSha")");

  TaskRunner runner;
  Storage storage(json, std::make_unique<FakeMapFilesDownloader>(runner));
  InitSpaStorage(storage, runner);

  CountryId const id = "SpaLeaf";
  TEST(!storage.GetCountryFile(id).HasRemoteSpa(), ());
  TEST_EQUAL(1024, storage.CountrySizeInBytes(id).second, ());

  storage.DeleteCountry(id, MapFileType::Map);
  DownloadAndPump(storage, runner, id, MapFileType::Map);

  auto localFile = storage.GetLatestLocalFile(id);
  TEST(localFile, ());
  localFile->SyncWithDisk();
  TEST(localFile->OnDisk(MapFileType::Map), ());
  TEST(!localFile->OnDisk(MapFileType::Spa), ());
  TEST_EQUAL(Status::OnDisk, storage.CountryStatusEx(id), ());
}

UNIT_TEST(Storage_SpaDownload_FailKeepsMap)
{
  WritableDirChanger const writableDirChanger(kMapTestDir);
  Platform::ThreadRunner threadRunner;

  auto const json = MakeSpaDownloadCountriesJson(R"(,
        "s": 2048,
        "sha1_base64": "mwmSha",
        "spa": 512,
        "spa_sha1_base64": "spaSha")");

  TaskRunner runner;
  Storage storage(json, std::make_unique<FakeMapFilesDownloader>(runner));
  InitSpaStorage(storage, runner);

  CountryId const id = "SpaLeaf";
  storage.DeleteCountry(id, MapFileType::Map);

  DownloadAndPump(storage, runner, id, MapFileType::Map);

  auto localFile = storage.GetLatestLocalFile(id);
  TEST(localFile, ());
  localFile->SyncWithDisk();
  TEST(localFile->OnDisk(MapFileType::Map), ());

  // Remove spa if the advertised pair already landed, then force a Spa-only failure.
  if (localFile->OnDisk(MapFileType::Spa))
  {
    localFile->DeleteFromDisk(MapFileType::Spa);
    localFile->SyncWithDisk();
  }
  TEST(!localFile->OnDisk(MapFileType::Spa), ());

  SpaDownloadDenyPolicy deny;
  storage.SetDownloadingPolicy(&deny);
  DownloadAndPump(storage, runner, id, MapFileType::Spa);

  localFile = storage.GetLatestLocalFile(id);
  TEST(localFile, ());
  localFile->SyncWithDisk();
  TEST(localFile->OnDisk(MapFileType::Map), ());
  TEST(!localFile->OnDisk(MapFileType::Spa), ());
  TEST_EQUAL(Status::OnDisk, storage.CountryStatusEx(id), ());
  TEST(!storage.CheckFailedCountries({id}), ());
}
}  // namespace
}  // namespace storage
