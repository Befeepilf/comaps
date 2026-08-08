#include "testing/testing.hpp"

#include "storage/storage.hpp"
#include "storage/downloading_policy.hpp"
#include "storage/storage_tests/fake_map_files_downloader.hpp"
#include "storage/storage_tests/task_runner.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"
#include "platform/settings.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include "base/file_name_utils.hpp"
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
using platform::tests_support::ScopedDir;
using platform::tests_support::ScopedFile;

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

UNIT_TEST(Storage_SpaDownload_RestoreQueueEnqueuesSpaWhenMapOnDisk)
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

  // Land Map (+Spa via advertise), then leave only Map on disk as after a Spa-pending restart.
  DownloadAndPump(storage, runner, id, MapFileType::Map);

  auto localFile = storage.GetLatestLocalFile(id);
  TEST(localFile, ());
  localFile->SyncWithDisk();
  TEST(localFile->OnDisk(MapFileType::Map), ());
  if (localFile->OnDisk(MapFileType::Spa))
  {
    localFile->DeleteFromDisk(MapFileType::Spa);
    localFile->SyncWithDisk();
  }
  TEST(!localFile->OnDisk(MapFileType::Spa), ());
  TEST_EQUAL(Status::OnDisk, storage.CountryStatusEx(id), ());

  // Saved queue contained this leaf while only Spa was still pending.
  settings::Set("DownloadQueue", id);
  storage.RestoreDownloadQueue();
  storage.StartPendingDownloadsForTesting();
  runner.Run();

  localFile = storage.GetLatestLocalFile(id);
  TEST(localFile, ());
  localFile->SyncWithDisk();
  TEST(localFile->OnDisk(MapFileType::Map), ());
  TEST(localFile->OnDisk(MapFileType::Spa), ());
  TEST_EQUAL(Status::OnDisk, storage.CountryStatusEx(id), ());
  TEST(!storage.CheckFailedCountries({id}), ());
}

UNIT_TEST(Storage_SpaLifecycle_DeleteCountryRemovesSpaKeepsPersonal)
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
  TEST(localFile->OnDisk(MapFileType::Spa), ());

  std::string const spaPath = localFile->GetPath(MapFileType::Spa);
  std::string const mapPath = localFile->GetPath(MapFileType::Map);

  // Personal exploration / assignment artifacts live in WritableDir (SPD-016 / SP-030).
  Platform & platform = GetPlatform();
  std::string const pixPath = base::JoinPath(platform.WritableDir(), id + PIX_FILE_EXTENSION);
  std::string const pixrPath = base::JoinPath(platform.WritableDir(), id + ".pixr");
  std::string const spxPath = base::JoinPath(platform.WritableDir(), id + SPX_FILE_EXTENSION);
  {
    FileWriter w(pixPath);
    std::string const bits = "pix-keep";
    w.Write(bits.data(), bits.size());
  }
  {
    FileWriter w(pixrPath);
    std::string const bits = "pixr-keep";
    w.Write(bits.data(), bits.size());
  }
  {
    FileWriter w(spxPath);
    std::string const bits = "spx-keep";
    w.Write(bits.data(), bits.size());
  }

  storage.DeleteCountry(id, MapFileType::Map);

  TEST(!Platform::IsFileExistsByFullPath(mapPath), ());
  TEST(!Platform::IsFileExistsByFullPath(spaPath), ());
  TEST(Platform::IsFileExistsByFullPath(pixPath), ());
  TEST(Platform::IsFileExistsByFullPath(pixrPath), ());
  TEST(Platform::IsFileExistsByFullPath(spxPath), ());
  TEST_EQUAL(Status::NotDownloaded, storage.CountryStatusEx(id), ());

  Platform::RemoveFileIfExists(pixPath);
  Platform::RemoveFileIfExists(pixrPath);
  Platform::RemoveFileIfExists(spxPath);
}

UNIT_TEST(Storage_SpaLifecycle_MapRedownloadRefetchesSpa)
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
  TEST(localFile->OnDisk(MapFileType::Spa), ());

  std::string const spaPath = localFile->GetPath(MapFileType::Spa);
  {
    FileWriter w(spaPath);
    std::string const stale = "STALE-SPA-MARKER!!!!!!!!!!!";
    w.Write(stale.data(), stale.size());
  }
  TEST(Platform::IsFileExistsByFullPath(spaPath), ());

  // Same-version Map replace must drop stale spa and full-refetch (SPD-029).
  DownloadAndPump(storage, runner, id, MapFileType::Map);

  localFile = storage.GetLatestLocalFile(id);
  TEST(localFile, ());
  localFile->SyncWithDisk();
  TEST(localFile->OnDisk(MapFileType::Map), ());
  TEST(localFile->OnDisk(MapFileType::Spa), ());
  TEST_EQUAL(512, localFile->GetSize(MapFileType::Spa), ());

  {
    FileReader reader(spaPath);
    std::string body;
    reader.ReadAsString(body);
    TEST_EQUAL(512, body.size(), ());
    TEST(body.find("STALE-SPA-MARKER") == std::string::npos, ());
  }
}

UNIT_TEST(Storage_SpaLifecycle_ObsoleteVersionRemovesSpa)
{
  WritableDirChanger const writableDirChanger(kMapTestDir);
  Platform::ThreadRunner threadRunner;

  CountryFile country("SpaLeaf");
  ScopedDir dir1(strings::to_string(version::FOR_TESTING_MWM1));
  ScopedFile map1(dir1, country, MapFileType::Map);
  ScopedFile spa1(dir1, country, MapFileType::Spa);
  LocalCountryFile file1(dir1.GetFullPath(), country, version::FOR_TESTING_MWM1);
  file1.SyncWithDisk();
  TEST(file1.OnDisk(MapFileType::Map), ());
  TEST(file1.OnDisk(MapFileType::Spa), ());

  ScopedDir dir2(strings::to_string(version::FOR_TESTING_MWM2));
  ScopedFile map2(dir2, country, MapFileType::Map);
  ScopedFile spa2(dir2, country, MapFileType::Spa);

  auto const json = MakeSpaDownloadCountriesJson(R"(,
        "s": 2048,
        "sha1_base64": "mwmSha",
        "spa": 512,
        "spa_sha1_base64": "spaSha")");

  TaskRunner runner;
  Storage storage(json, std::make_unique<FakeMapFilesDownloader>(runner));
  InitSpaStorage(storage, runner);
  storage.SetCurrentDataVersionForTesting(version::FOR_TESTING_MWM2);

  std::string const oldSpaPath = file1.GetPath(MapFileType::Spa);
  TEST(Platform::IsFileExistsByFullPath(oldSpaPath), ());

  storage.RegisterAllLocalMaps();

  TEST(!map1.Exists(), ());
  TEST(!Platform::IsFileExistsByFullPath(oldSpaPath), ());
  map1.Reset();
  spa1.Reset();

  TEST(map2.Exists(), ());
  TEST(spa2.Exists(), ());
}

UNIT_TEST(Storage_SpaIncomplete_FailSoftMarksIncomplete)
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
  if (localFile->OnDisk(MapFileType::Spa))
  {
    localFile->DeleteFromDisk(MapFileType::Spa);
    localFile->SyncWithDisk();
  }
  TEST(!localFile->OnDisk(MapFileType::Spa), ());
  TEST(!storage.IsSpaIncomplete(id), ());

  SpaDownloadDenyPolicy deny;
  storage.SetDownloadingPolicy(&deny);
  DownloadAndPump(storage, runner, id, MapFileType::Spa);

  TEST_EQUAL(Status::OnDisk, storage.CountryStatusEx(id), ());
  TEST(!storage.CheckFailedCountries({id}), ());
  TEST(storage.IsSpaIncomplete(id), ());

  CountriesVec incomplete;
  storage.GetIncompleteSpaCountries(incomplete);
  TEST_EQUAL(1, incomplete.size(), ());
  TEST_EQUAL(id, incomplete.front(), ());

  // Fail-closed precondition: no spa bytes on disk for TryLoad to invent areas from.
  localFile = storage.GetLatestLocalFile(id);
  TEST(localFile, ());
  localFile->SyncWithDisk();
  TEST(!localFile->OnDisk(MapFileType::Spa), ());
  TEST(!Platform::IsFileExistsByFullPath(localFile->GetPath(MapFileType::Spa)), ());
}

UNIT_TEST(Storage_SpaIncomplete_RetryClearsAfterSpaDownload)
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
  if (localFile->OnDisk(MapFileType::Spa))
  {
    localFile->DeleteFromDisk(MapFileType::Spa);
    localFile->SyncWithDisk();
  }

  SpaDownloadDenyPolicy deny;
  storage.SetDownloadingPolicy(&deny);
  DownloadAndPump(storage, runner, id, MapFileType::Spa);
  TEST(storage.IsSpaIncomplete(id), ());
  TEST(!localFile->OnDisk(MapFileType::Spa), ());

  // Allow download again and retry without re-fetching the MWM.
  DownloadingPolicy allow;
  storage.SetDownloadingPolicy(&allow);
  storage.RetryIncompleteSpaDownloads();
  storage.StartPendingDownloadsForTesting();
  runner.Run();

  localFile = storage.GetLatestLocalFile(id);
  TEST(localFile, ());
  localFile->SyncWithDisk();
  TEST(localFile->OnDisk(MapFileType::Map), ());
  TEST(localFile->OnDisk(MapFileType::Spa), ());
  TEST(!storage.IsSpaIncomplete(id), ());
  TEST_EQUAL(Status::OnDisk, storage.CountryStatusEx(id), ());
}

UNIT_TEST(Storage_SpaIncomplete_MissingMetaNeverIncomplete)
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

  storage.DeleteCountry(id, MapFileType::Map);
  DownloadAndPump(storage, runner, id, MapFileType::Map);

  auto localFile = storage.GetLatestLocalFile(id);
  TEST(localFile, ());
  localFile->SyncWithDisk();
  TEST(localFile->OnDisk(MapFileType::Map), ());
  TEST(!localFile->OnDisk(MapFileType::Spa), ());
  TEST(!storage.IsSpaIncomplete(id), ());

  // Retry must not invent advertisement or mark incomplete.
  storage.RetryIncompleteSpaDownloads();
  storage.StartPendingDownloadsForTesting();
  runner.Run();

  TEST(!storage.IsSpaIncomplete(id), ());
  CountriesVec incomplete;
  storage.GetIncompleteSpaCountries(incomplete);
  TEST(incomplete.empty(), ());
  localFile = storage.GetLatestLocalFile(id);
  TEST(localFile, ());
  localFile->SyncWithDisk();
  TEST(!localFile->OnDisk(MapFileType::Spa), ());
}

UNIT_TEST(Storage_SpaIncomplete_RestoreQueueAutoRetries)
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
  if (localFile->OnDisk(MapFileType::Spa))
  {
    localFile->DeleteFromDisk(MapFileType::Spa);
    localFile->SyncWithDisk();
  }

  SpaDownloadDenyPolicy deny;
  storage.SetDownloadingPolicy(&deny);
  DownloadAndPump(storage, runner, id, MapFileType::Spa);
  TEST(storage.IsSpaIncomplete(id), ());

  // Persist incomplete flag, then simulate cold restore with downloads allowed.
  DownloadingPolicy allow;
  storage.SetDownloadingPolicy(&allow);
  settings::Set("DownloadQueue", std::string());
  storage.RestoreDownloadQueue();
  storage.StartPendingDownloadsForTesting();
  runner.Run();

  localFile = storage.GetLatestLocalFile(id);
  TEST(localFile, ());
  localFile->SyncWithDisk();
  TEST(localFile->OnDisk(MapFileType::Spa), ());
  TEST(!storage.IsSpaIncomplete(id), ());
}

UNIT_TEST(Storage_GetNodeAttrs_GroupSubtreeDoesNotAbort)
{
  WritableDirChanger const writableDirChanger(kMapTestDir);
  Platform::ThreadRunner threadRunner;

  auto const json = std::string(R"({
    "id": "Countries",
    "v": )") +
                    strings::to_string(version::FOR_TESTING_MWM1) + R"(,
    "g": [
      {
        "id": "RegionGroup",
        "g": [
          {
            "id": "SpaLeaf",
            "s": 2048,
            "sha1_base64": "mwmSha",
            "spa": 512,
            "spa_sha1_base64": "spaSha"
          }
        ]
      }
    ]
  })";

  TaskRunner runner;
  Storage storage(json, std::make_unique<FakeMapFilesDownloader>(runner));
  InitSpaStorage(storage, runner);

  NodeAttrs attrs;
  storage.GetNodeAttrs("RegionGroup", attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::NotDownloaded, ());
  TEST_EQUAL(attrs.m_mwmCounter, 1, ());
  TEST_EQUAL(attrs.m_mwmSize, 2048 + 512, ());
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, 0, ());

  storage.GetNodeAttrs(storage.GetRootId(), attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::NotDownloaded, ());
  TEST_EQUAL(attrs.m_mwmCounter, 1, ());
  TEST_EQUAL(attrs.m_mwmSize, 2048 + 512, ());
}
}  // namespace
}  // namespace storage
