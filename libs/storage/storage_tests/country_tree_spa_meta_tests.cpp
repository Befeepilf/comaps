#include "testing/testing.hpp"

#include "storage/country_tree.hpp"
#include "storage/storage.hpp"
#include "storage/storage_tests/test_map_files_downloader.hpp"

#include "platform/country_file.hpp"
#include "platform/mwm_version.hpp"

#include "base/string_utils.hpp"

#include <memory>
#include <string>

namespace storage
{
namespace
{
using platform::CountryFile;

std::string MakeCountriesJson(std::string const & leafFields)
{
  return std::string(R"({
    "id": "Countries",
    "v": )") +
         strings::to_string(version::FOR_TESTING_MWM1) + R"(,
    "g": [
      {
        "id": "SpaMetaLeaf")" +
         leafFields + R"(
      },
      {
        "id": "PlainLeaf",
        "s": 100,
        "sha1_base64": "plainSha"
      }
    ]
  })";
}

CountryFile LoadLeafFile(std::string const & json, CountryId const & id)
{
  Storage storage(json, std::make_unique<TestMapFilesDownloader>());
  return storage.GetCountryFile(id);
}
}  // namespace

UNIT_TEST(CountryTree_SpaMeta_Absent)
{
  auto const json = MakeCountriesJson(R"(,
        "s": 200,
        "sha1_base64": "mwmSha")");
  auto const file = LoadLeafFile(json, "SpaMetaLeaf");
  TEST_EQUAL(200, file.GetRemoteSize(), ());
  TEST_EQUAL("mwmSha", file.GetSha1(), ());
  TEST_EQUAL(0, file.GetRemoteSpaSize(), ());
  TEST(file.GetSpaSha1().empty(), ());
  TEST(!file.HasRemoteSpa(), ());

  // Subtree size is MWM `"s"` only when spa is not advertised.
  Storage storage(json, std::make_unique<TestMapFilesDownloader>());
  TEST_EQUAL(200, storage.CountryLeafByCountryId("SpaMetaLeaf").GetSubtreeMwmSizeBytes(), ());
}

UNIT_TEST(CountryTree_SpaMeta_BothPresent)
{
  auto const json = MakeCountriesJson(R"(,
        "s": 200,
        "sha1_base64": "mwmSha",
        "spa": 42,
        "spa_sha1_base64": "spaSha")");
  auto const file = LoadLeafFile(json, "SpaMetaLeaf");
  TEST_EQUAL(200, file.GetRemoteSize(), ());
  TEST_EQUAL("mwmSha", file.GetSha1(), ());
  TEST_EQUAL(42, file.GetRemoteSpaSize(), ());
  TEST_EQUAL("spaSha", file.GetSpaSha1(), ());
  TEST(file.HasRemoteSpa(), ());

  Storage storage(json, std::make_unique<TestMapFilesDownloader>());
  // Advertised spa bytes fold into subtree / Android totalSize (SP-046).
  TEST_EQUAL(242, storage.CountryLeafByCountryId("SpaMetaLeaf").GetSubtreeMwmSizeBytes(), ());
  TEST_EQUAL(100, storage.CountryLeafByCountryId("PlainLeaf").GetSubtreeMwmSizeBytes(), ());
}

UNIT_TEST(CountryTree_SpaMeta_UnknownKeysForwardCompat)
{
  auto const json = MakeCountriesJson(R"(,
        "s": 200,
        "sha1_base64": "mwmSha",
        "spa": 7,
        "spa_sha1_base64": "spaSha",
        "future_key": 1,
        "another_future": "x")");
  auto const file = LoadLeafFile(json, "SpaMetaLeaf");
  TEST_EQUAL(200, file.GetRemoteSize(), ());
  TEST_EQUAL("mwmSha", file.GetSha1(), ());
  TEST_EQUAL(7, file.GetRemoteSpaSize(), ());
  TEST_EQUAL("spaSha", file.GetSpaSha1(), ());
  TEST(file.HasRemoteSpa(), ());
}

UNIT_TEST(CountryTree_SpaMeta_PartialSpaOnly)
{
  auto const json = MakeCountriesJson(R"(,
        "s": 200,
        "sha1_base64": "mwmSha",
        "spa": 42)");
  auto const file = LoadLeafFile(json, "SpaMetaLeaf");
  TEST_EQUAL(200, file.GetRemoteSize(), ());
  TEST(!file.HasRemoteSpa(), ());
  TEST_EQUAL(0, file.GetRemoteSpaSize(), ());
  TEST(file.GetSpaSha1().empty(), ());
}

UNIT_TEST(CountryTree_SpaMeta_PartialHashOnly)
{
  auto const json = MakeCountriesJson(R"(,
        "s": 200,
        "sha1_base64": "mwmSha",
        "spa_sha1_base64": "spaSha")");
  auto const file = LoadLeafFile(json, "SpaMetaLeaf");
  TEST_EQUAL(200, file.GetRemoteSize(), ());
  TEST(!file.HasRemoteSpa(), ());
  TEST_EQUAL(0, file.GetRemoteSpaSize(), ());
  TEST(file.GetSpaSha1().empty(), ());
}

UNIT_TEST(CountryTree_SpaMeta_ZeroSpaWithHash)
{
  auto const json = MakeCountriesJson(R"(,
        "s": 200,
        "sha1_base64": "mwmSha",
        "spa": 0,
        "spa_sha1_base64": "spaSha")");
  auto const file = LoadLeafFile(json, "SpaMetaLeaf");
  TEST(!file.HasRemoteSpa(), ());
  TEST_EQUAL(0, file.GetRemoteSpaSize(), ());
  TEST(file.GetSpaSha1().empty(), ());
}
}  // namespace storage
