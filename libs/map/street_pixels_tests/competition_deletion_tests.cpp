#include "testing/testing.hpp"

#include "map/backend_config.hpp"
#include "map/identity_store.hpp"
#include "map/street_pixels_file.hpp"

#include "street_pixels_areas/live_recency_store.hpp"

#include "platform/platform.hpp"
#include "platform/secure_storage.hpp"
#include "platform/settings.hpp"

#include "base/file_name_utils.hpp"

#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string Writable(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void RemoveDeletionRecencyDb(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
  Platform::RemoveFileIfExists(path + "-wal");
  Platform::RemoveFileIfExists(path + "-shm");
}

void ClearDeletionIdentityKeys()
{
  settings::Delete("Explore.ConsentGiven");
  settings::Delete("Explore.CompetitionEnabled");
  settings::Delete("Explore.AggregateSharingEnabled");
  settings::Delete("Explore.ConsentPolicyVersion");
  settings::Delete("Explore.ConsentUnixTime");
  settings::Delete("Explore.Username");
  settings::Delete("Explore.NicknameDraft");
  settings::Delete("Explore.NicknameLastChangedUnix");
  backend::SetApiBaseUrl("");
  IdentityStore::SetNicknameClaimHandlerForTesting({});
  IdentityStore::SetNicknameClaimPostFnForTesting({});
  IdentityStore::SetCompetitionConsentGrantedHandler({});
  platform::SecureStorage storage;
  storage.Remove("Explore.DeviceId");
}

class ScopedDeletionFixture
{
public:
  static std::int64_t constexpr kPixelId = 1000;

  ScopedDeletionFixture()
    : m_pixPath(Writable("sp077_delete.pix"))
    , m_dbPath(Writable("sp077_delete_recency.db"))
    , m_store(m_dbPath)
  {
    ClearDeletionIdentityKeys();
    Platform::RemoveFileIfExists(m_pixPath);
    RemoveDeletionRecencyDb(m_dbPath);
    m_store.Reopen(m_dbPath);

    street_pixels_file::ExploredEverLiveMap seed{{kPixelId, true}};
    TEST(street_pixels_file::SaveRematchedUniverse(m_pixPath, std::set<int64_t>{kPixelId}, seed, 42), ());
    m_before = street_pixels_file::ProbeFile(m_pixPath);
    m_store.TouchLiveVisits({kPixelId}, 12345);
    TEST_EQUAL(*m_store.GetLastLiveVisit(kPixelId), 12345, ());

    IdentityStore::GrantCompetitionConsent();
    IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 200; });
    TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
    backend::SetApiBaseUrl("https://example.com/api");
  }

  ~ScopedDeletionFixture()
  {
    ClearDeletionIdentityKeys();
    Platform::RemoveFileIfExists(m_pixPath);
    RemoveDeletionRecencyDb(m_dbPath);
  }

  std::string const & PixPath() const { return m_pixPath; }
  street_pixels_file::ProbeResult const & Before() const { return m_before; }
  street_pixels::LiveRecencyStore & Store() { return m_store; }

private:
  std::string m_pixPath;
  std::string m_dbPath;
  street_pixels_file::ProbeResult m_before;
  street_pixels::LiveRecencyStore m_store;
};
}  // namespace

UNIT_TEST(CompetitionDeletion_SuccessDoesNotClearPixOrRecency)
{
  ScopedDeletionFixture fixture;
  IdentityStore::SetNicknameClaimPostFnForTesting(
      [](std::string const &, std::string const &,
         std::vector<std::pair<std::string, std::string>> const & headers)
      {
        TEST(headers.empty(), ());
        return 200;
      });
  TEST(IdentityStore::DeleteCompetitionProfile() == IdentityStore::CompetitionAccountResult::Ok, ());
  auto const after = street_pixels_file::ProbeFile(fixture.PixPath());
  TEST_EQUAL(static_cast<int>(after.kind), static_cast<int>(fixture.Before().kind), ());
  TEST_EQUAL(*fixture.Store().GetLastLiveVisit(ScopedDeletionFixture::kPixelId), 12345, ());
  TEST(!IdentityStore::HasUsername(), ());
  TEST(!IdentityStore::HasCompetitionConsent(), ());
}

UNIT_TEST(CompetitionDeletion_FailedKeepsPixRecencyAndIdentity)
{
  ScopedDeletionFixture fixture;
  IdentityStore::SetNicknameClaimPostFnForTesting(
      [](std::string const &, std::string const &,
         std::vector<std::pair<std::string, std::string>> const &) { return 500; });
  TEST(IdentityStore::DeleteCompetitionProfile() == IdentityStore::CompetitionAccountResult::Unavailable, ());
  auto const after = street_pixels_file::ProbeFile(fixture.PixPath());
  TEST_EQUAL(static_cast<int>(after.kind), static_cast<int>(fixture.Before().kind), ());
  TEST_EQUAL(*fixture.Store().GetLastLiveVisit(ScopedDeletionFixture::kPixelId), 12345, ());
  TEST(IdentityStore::HasCompetitionConsent(), ());
  TEST_EQUAL(IdentityStore::GetUsername(), "Alice_1", ());
}
