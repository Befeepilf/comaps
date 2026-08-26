#include "testing/testing.hpp"

#include "map/backend_config.hpp"
#include "map/identity_store.hpp"

#include "platform/secure_storage.hpp"
#include "platform/settings.hpp"

#include "base/timer.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
void ClearIdentityKeys()
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

class ScopedIdentitySettings
{
public:
  ScopedIdentitySettings() { ClearIdentityKeys(); }
  ~ScopedIdentitySettings() { ClearIdentityKeys(); }
};

bool HasNoAsciiDigits(std::string const & s)
{
  return s.find_first_of("0123456789") == std::string::npos;
}

void ExpectInvalidAndUnpersisted(std::string_view nickname)
{
  TEST(!IdentityStore::IsValidNickname(nickname), ());
  TEST(IdentityStore::TryClaimNickname(nickname) == IdentityStore::NicknameClaimResult::Invalid, ());
  TEST(!IdentityStore::HasUsername(), ());
  TEST(IdentityStore::GetUsername().empty(), ());
}

bool IdentityJsonHasQuotedKey(std::string const & json, std::string_view key)
{
  std::string const needle = "\"" + std::string(key) + "\"";
  return json.find(needle) != std::string::npos;
}

bool HasForbiddenClaimHeader(std::vector<std::pair<std::string, std::string>> const & headers)
{
  for (auto const & header : headers)
  {
    std::string name = header.first;
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (name == "x-device-id" || name == "x-username")
      return true;
    if (name.find("friend") != std::string::npos)
      return true;
  }
  return false;
}
}  // namespace

UNIT_TEST(IdentityStore_LegacyConsentBooleanIsNotConsent)
{
  ScopedIdentitySettings scoped;
  settings::Set("Explore.ConsentGiven", true);
  TEST(!IdentityStore::HasCompetitionConsent(), ());
  TEST(!IdentityStore::HasExploreConsent(), ());
  TEST(!IdentityStore::ShouldUploadCompetitionIdentity(), ());
}

UNIT_TEST(IdentityStore_RejectsInvalidNicknames)
{
  ScopedIdentitySettings scoped;
  ExpectInvalidAndUnpersisted("");
  ExpectInvalidAndUnpersisted("ab");
  ExpectInvalidAndUnpersisted(std::string(25, 'a'));
  ExpectInvalidAndUnpersisted("a\nb");
  ExpectInvalidAndUnpersisted("hi\t");
  ExpectInvalidAndUnpersisted("http://x.com");
  ExpectInvalidAndUnpersisted("www.x.com");
  ExpectInvalidAndUnpersisted("a@b.c");
  ExpectInvalidAndUnpersisted("12345678");
  ExpectInvalidAndUnpersisted("+12345678");
  ExpectInvalidAndUnpersisted("+1 (234) 567-890");
  ExpectInvalidAndUnpersisted(std::string("aa\x80", 3));
  ExpectInvalidAndUnpersisted("aaa\x07");
  std::string tooManyCombining = "a";
  for (int i = 0; i < 8; ++i)
    tooManyCombining += "\xCC\x81";
  ExpectInvalidAndUnpersisted(tooManyCombining);
  ExpectInvalidAndUnpersisted(" ");
  ExpectInvalidAndUnpersisted("google.com");
  ExpectInvalidAndUnpersisted("555-123-4567");
  ExpectInvalidAndUnpersisted("555 123 4567");
  std::string zwsp = "aa";
  zwsp += "\xE2\x80\x8B";
  zwsp += "b";
  ExpectInvalidAndUnpersisted(zwsp);
}

UNIT_TEST(IdentityStore_AcceptsUnicodeNicknames)
{
  ScopedIdentitySettings scoped;
  TEST(IdentityStore::IsValidNickname("abc"), ());
  TEST(IdentityStore::IsValidNickname(std::string(24, 'a')), ());
  TEST(IdentityStore::IsValidNickname("Alice_1"), ());
  TEST(IdentityStore::IsValidNickname("José-María"), ());
  TEST(IdentityStore::IsValidNickname("東京太郎"), ());
  TEST(IdentityStore::IsValidNickname("Anna Birch"), ());
  TEST(IdentityStore::IsValidNickname("O'Neil"), ());
  std::string const generated = IdentityStore::GenerateNickname();
  TEST(IdentityStore::IsValidNickname(generated), ());
  TEST(HasNoAsciiDigits(generated), ());
}

UNIT_TEST(IdentityStore_Collision409DoesNotPersistNickname)
{
  ScopedIdentitySettings scoped;
  IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 409; });
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Collision, ());
  TEST(!IdentityStore::HasUsername(), ());
  TEST(IdentityStore::GetUsername().empty(), ());

  IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 200; });
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
  uint64_t const aged = base::SecondsSinceEpoch() - IdentityStore::kNicknameRenameIntervalSeconds;
  settings::Set("Explore.NicknameLastChangedUnix", aged);
  IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 409; });
  TEST(IdentityStore::TryClaimNickname("Bob_2") == IdentityStore::NicknameClaimResult::Collision, ());
  TEST_EQUAL(IdentityStore::GetUsername(), "Alice_1", ());
}

UNIT_TEST(IdentityStore_ConsentOffBlocksIdentityUpload)
{
  ScopedIdentitySettings scoped;
  TEST(!IdentityStore::ShouldUploadCompetitionIdentity(), ());
  TEST_EQUAL(IdentityStore::GetCompetitionConsentUnixTime(), 0, ());
  IdentityStore::GrantCompetitionConsent();
  TEST(IdentityStore::HasCompetitionConsent(), ());
  TEST(IdentityStore::ShouldUploadCompetitionIdentity(), ());
  TEST(IdentityStore::GetCompetitionConsentUnixTime() != 0, ());
  IdentityStore::RevokeCompetitionConsent();
  TEST(!IdentityStore::ShouldUploadCompetitionIdentity(), ());
  TEST_EQUAL(IdentityStore::GetCompetitionConsentUnixTime(), 0, ());
}

UNIT_TEST(IdentityStore_GrantInvokesHandlerWithStoredUnixTime)
{
  ScopedIdentitySettings scoped;
  uint64_t observed = 0;
  size_t calls = 0;
  IdentityStore::SetCompetitionConsentGrantedHandler([&](uint64_t unixTime)
  {
    ++calls;
    observed = unixTime;
  });
  uint64_t const before = base::SecondsSinceEpoch();
  IdentityStore::GrantCompetitionConsent();
  uint64_t const after = base::SecondsSinceEpoch();
  TEST_EQUAL(calls, 1, ());
  TEST_EQUAL(observed, IdentityStore::GetCompetitionConsentUnixTime(), ());
  TEST(observed >= before, ());
  TEST(observed <= after, ());
}

UNIT_TEST(IdentityStore_GrantWithoutHandlerDoesNotCrash)
{
  ScopedIdentitySettings scoped;
  IdentityStore::SetCompetitionConsentGrantedHandler({});
  IdentityStore::GrantCompetitionConsent();
  TEST(IdentityStore::HasCompetitionConsent(), ());
  TEST(IdentityStore::GetCompetitionConsentUnixTime() != 0, ());
}

UNIT_TEST(IdentityStore_EmptyPolicyVersionIsNotConsent)
{
  ScopedIdentitySettings scoped;
  settings::Set("Explore.CompetitionEnabled", true);
  settings::Set("Explore.AggregateSharingEnabled", true);
  settings::Set("Explore.ConsentPolicyVersion", std::string());
  settings::Set("Explore.ConsentUnixTime", uint64_t{1});
  TEST(!IdentityStore::HasCompetitionConsent(), ());
  settings::Set("Explore.ConsentPolicyVersion", std::string(IdentityStore::kCompetitionPrivacyPolicyVersion));
  TEST(IdentityStore::HasCompetitionConsent(), ());
}

UNIT_TEST(IdentityStore_InvalidUtf8NicknameRejected)
{
  ScopedIdentitySettings scoped;
  std::string const invalid("aa\x80", 3);
  TEST(!IdentityStore::IsValidNickname(invalid), ());
  TEST(IdentityStore::TryClaimNickname(invalid) == IdentityStore::NicknameClaimResult::Invalid, ());
  TEST(!IdentityStore::HasUsername(), ());
  TEST(IdentityStore::GetUsername().empty(), ());
}

UNIT_TEST(IdentityStore_ExistingAsciiUsernameNotAutoAccepted)
{
  ScopedIdentitySettings scoped;
  settings::Set("Explore.Username", std::string("old_user"));
  TEST(!IdentityStore::HasUsername(), ());
  TEST(IdentityStore::GetUsername().empty(), ());
  TEST_EQUAL(IdentityStore::GetNicknameDraft(), "old_user", ());
  IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 200; });
  TEST(IdentityStore::TryClaimNickname("old_user") == IdentityStore::NicknameClaimResult::Ok, ());
  TEST(IdentityStore::HasUsername(), ());
  TEST_EQUAL(IdentityStore::GetUsername(), "old_user", ());
}

UNIT_TEST(IdentityStore_RenameLimitLocalSevenDays)
{
  ScopedIdentitySettings scoped;
  IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 200; });
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
  TEST(!IdentityStore::CanRenameNickname(), ());
  TEST(IdentityStore::TryClaimNickname("Alice_2") == IdentityStore::NicknameClaimResult::RenameLimited, ());
  TEST_EQUAL(IdentityStore::GetUsername(), "Alice_1", ());
  uint64_t const aged = base::SecondsSinceEpoch() - IdentityStore::kNicknameRenameIntervalSeconds;
  settings::Set("Explore.NicknameLastChangedUnix", aged);
  TEST(IdentityStore::CanRenameNickname(), ());
  TEST(IdentityStore::TryClaimNickname("Alice_2") == IdentityStore::NicknameClaimResult::Ok, ());
  TEST_EQUAL(IdentityStore::GetUsername(), "Alice_2", ());
}

UNIT_TEST(IdentityStore_GenerateNicknameRetryIsNotNumericSuffix)
{
  ScopedIdentitySettings scoped;
  std::string const first = IdentityStore::GenerateNickname(0);
  std::string const second = IdentityStore::GenerateNickname(1);
  TEST(IdentityStore::IsValidNickname(first), ());
  TEST(IdentityStore::IsValidNickname(second), ());
  TEST(HasNoAsciiDigits(first), ());
  TEST(HasNoAsciiDigits(second), ());
  std::string const generated = IdentityStore::GenerateNickname();
  TEST(IdentityStore::IsValidNickname(generated), ());
  TEST(HasNoAsciiDigits(generated), ());
}

UNIT_TEST(IdentityStore_UnsetHandlerKeepsDraftOnly)
{
  ScopedIdentitySettings scoped;
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Unavailable, ());
  TEST(!IdentityStore::HasUsername(), ());
  TEST(IdentityStore::GetUsername().empty(), ());
  TEST_EQUAL(IdentityStore::GetNicknameDraft(), "Alice_1", ());
}

UNIT_TEST(IdentityStore_SetUsernameDoesNotAccept)
{
  ScopedIdentitySettings scoped;
  TEST(IdentityStore::SetUsername("Alice_1"), ());
  TEST(!IdentityStore::HasUsername(), ());
  TEST(IdentityStore::GetUsername().empty(), ());
  TEST_EQUAL(IdentityStore::GetNicknameDraft(), "Alice_1", ());
}

UNIT_TEST(IdentityStore_SameNicknameReclaimIsNotLimited)
{
  ScopedIdentitySettings scoped;
  IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 200; });
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
  TEST(!IdentityStore::CanRenameNickname(), ());
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
  TEST_EQUAL(IdentityStore::GetUsername(), "Alice_1", ());
}

UNIT_TEST(IdentityStore_ProductionClaimEmptyApiNoHttp)
{
  ScopedIdentitySettings scoped;
  IdentityStore::GrantCompetitionConsent();
  backend::SetApiBaseUrl("");
  IdentityStore::SetNicknameClaimHandlerForTesting(&IdentityStore::PostNicknameClaim);
  int posts = 0;
  IdentityStore::SetNicknameClaimPostFnForTesting(
      [&](std::string const &, std::string const &,
          std::vector<std::pair<std::string, std::string>> const &)
      {
        ++posts;
        return 200;
      });
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Unavailable, ());
  TEST_EQUAL(posts, 0, ());
  TEST(!IdentityStore::HasUsername(), ());
  TEST_EQUAL(IdentityStore::GetNicknameDraft(), "Alice_1", ());
}

UNIT_TEST(IdentityStore_ProductionClaimPostsRegisterJsonWithoutFriendsHeaders)
{
  ScopedIdentitySettings scoped;
  IdentityStore::GrantCompetitionConsent();
  backend::SetApiBaseUrl("https://example.com/api");
  IdentityStore::SetNicknameClaimHandlerForTesting(&IdentityStore::PostNicknameClaim);
  int posts = 0;
  std::string lastUrl;
  std::string lastBody;
  std::vector<std::pair<std::string, std::string>> lastHeaders;
  IdentityStore::SetNicknameClaimPostFnForTesting(
      [&](std::string const & url, std::string const & body,
          std::vector<std::pair<std::string, std::string>> const & headers)
      {
        ++posts;
        lastUrl = url;
        lastBody = body;
        lastHeaders = headers;
        return 200;
      });
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
  TEST_EQUAL(posts, 1, ());
  TEST_EQUAL(lastUrl, backend::GetCompetitionRegisterUrl(), ());
  TEST_EQUAL(lastUrl, "https://example.com/api/v1/competition/register", ());
  TEST(IdentityJsonHasQuotedKey(lastBody, "profile_id"), (lastBody));
  TEST(IdentityJsonHasQuotedKey(lastBody, "nickname"), (lastBody));
  TEST(IdentityJsonHasQuotedKey(lastBody, "consent_policy_version"), (lastBody));
  TEST(IdentityJsonHasQuotedKey(lastBody, "consent_unix"), (lastBody));
  TEST(lastBody.find("Alice_1") != std::string::npos, (lastBody));
  TEST(lastBody.find(IdentityStore::GetOrCreateDeviceId()) != std::string::npos, (lastBody));
  TEST(!HasForbiddenClaimHeader(lastHeaders), ());
  TEST(lastHeaders.empty(), ());
  TEST(IdentityStore::HasUsername(), ());
  TEST_EQUAL(IdentityStore::GetUsername(), "Alice_1", ());
}

UNIT_TEST(IdentityStore_DefaultHandlerPostsWhenApiConfigured)
{
  ScopedIdentitySettings scoped;
  IdentityStore::GrantCompetitionConsent();
  backend::SetApiBaseUrl("https://example.com/api");
  int posts = 0;
  std::string lastUrl;
  std::vector<std::pair<std::string, std::string>> lastHeaders;
  IdentityStore::SetNicknameClaimPostFnForTesting(
      [&](std::string const & url, std::string const &,
          std::vector<std::pair<std::string, std::string>> const & headers)
      {
        ++posts;
        lastUrl = url;
        lastHeaders = headers;
        return 200;
      });
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
  TEST_EQUAL(posts, 1, ());
  TEST_EQUAL(lastUrl, "https://example.com/api/v1/competition/register", ());
  TEST(!HasForbiddenClaimHeader(lastHeaders), ());
  TEST(lastHeaders.empty(), ());
  TEST_EQUAL(IdentityStore::GetUsername(), "Alice_1", ());
}

UNIT_TEST(IdentityStore_ProductionClaimRenameUsesNicknameUrl)
{
  ScopedIdentitySettings scoped;
  IdentityStore::GrantCompetitionConsent();
  backend::SetApiBaseUrl("https://example.com/api");
  IdentityStore::SetNicknameClaimHandlerForTesting(&IdentityStore::PostNicknameClaim);
  std::string lastUrl;
  IdentityStore::SetNicknameClaimPostFnForTesting(
      [&](std::string const & url, std::string const &,
          std::vector<std::pair<std::string, std::string>> const & headers)
      {
        TEST(!HasForbiddenClaimHeader(headers), ());
        lastUrl = url;
        return 200;
      });
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
  TEST_EQUAL(lastUrl, backend::GetCompetitionRegisterUrl(), ());
  uint64_t const aged = base::SecondsSinceEpoch() - IdentityStore::kNicknameRenameIntervalSeconds;
  settings::Set("Explore.NicknameLastChangedUnix", aged);
  TEST(IdentityStore::TryClaimNickname("Alice Birch") == IdentityStore::NicknameClaimResult::Ok, ());
  TEST_EQUAL(lastUrl, backend::GetCompetitionNicknameUrl(), ());
  TEST_EQUAL(lastUrl, "https://example.com/api/v1/competition/nickname", ());
  TEST_EQUAL(IdentityStore::GetUsername(), "Alice Birch", ());
}

UNIT_TEST(IdentityStore_LeaveCompetitionRetainKeepsUsername)
{
  ScopedIdentitySettings scoped;
  IdentityStore::GrantCompetitionConsent();
  IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 200; });
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
  backend::SetApiBaseUrl("https://example.com/api");
  int posts = 0;
  std::string lastUrl;
  std::string lastBody;
  std::vector<std::pair<std::string, std::string>> lastHeaders;
  IdentityStore::SetNicknameClaimPostFnForTesting(
      [&](std::string const & url, std::string const & body,
          std::vector<std::pair<std::string, std::string>> const & headers)
      {
        ++posts;
        lastUrl = url;
        lastBody = body;
        lastHeaders = headers;
        return 200;
      });
  TEST(IdentityStore::LeaveCompetitionRetain() == IdentityStore::CompetitionAccountResult::Ok, ());
  TEST_EQUAL(posts, 1, ());
  TEST_EQUAL(lastUrl, backend::GetCompetitionLeaveUrl(), ());
  TEST(IdentityJsonHasQuotedKey(lastBody, "profile_id"), (lastBody));
  TEST(!HasForbiddenClaimHeader(lastHeaders), ());
  TEST(lastHeaders.empty(), ());
  TEST(!IdentityStore::HasCompetitionConsent(), ());
  TEST_EQUAL(IdentityStore::GetUsername(), "Alice_1", ());
}

UNIT_TEST(IdentityStore_LeaveCompetitionRetainFailedHttpStillRevokes)
{
  ScopedIdentitySettings scoped;
  IdentityStore::GrantCompetitionConsent();
  IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 200; });
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
  backend::SetApiBaseUrl("https://example.com/api");
  IdentityStore::SetNicknameClaimPostFnForTesting(
      [](std::string const &, std::string const &,
         std::vector<std::pair<std::string, std::string>> const &) { return 500; });
  TEST(IdentityStore::LeaveCompetitionRetain() == IdentityStore::CompetitionAccountResult::Unavailable, ());
  TEST(!IdentityStore::HasCompetitionConsent(), ());
  TEST_EQUAL(IdentityStore::GetUsername(), "Alice_1", ());
}

UNIT_TEST(IdentityStore_DeleteCompetitionProfileClearsUsername)
{
  ScopedIdentitySettings scoped;
  IdentityStore::GrantCompetitionConsent();
  IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 200; });
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
  backend::SetApiBaseUrl("https://example.com/api");
  IdentityStore::SetNicknameClaimPostFnForTesting(
      [](std::string const &, std::string const &,
         std::vector<std::pair<std::string, std::string>> const & headers)
      {
        TEST(!HasForbiddenClaimHeader(headers), ());
        TEST(headers.empty(), ());
        return 200;
      });
  TEST(IdentityStore::DeleteCompetitionProfile() == IdentityStore::CompetitionAccountResult::Ok, ());
  TEST(!IdentityStore::HasCompetitionConsent(), ());
  TEST(!IdentityStore::HasUsername(), ());
  TEST(IdentityStore::GetUsername().empty(), ());
}

UNIT_TEST(IdentityStore_DeleteCompetitionProfileFailedKeepsIdentity)
{
  ScopedIdentitySettings scoped;
  IdentityStore::GrantCompetitionConsent();
  IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 200; });
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
  backend::SetApiBaseUrl("https://example.com/api");
  IdentityStore::SetNicknameClaimPostFnForTesting(
      [](std::string const &, std::string const &,
         std::vector<std::pair<std::string, std::string>> const &) { return 500; });
  TEST(IdentityStore::DeleteCompetitionProfile() == IdentityStore::CompetitionAccountResult::Unavailable, ());
  TEST(IdentityStore::HasCompetitionConsent(), ());
  TEST_EQUAL(IdentityStore::GetUsername(), "Alice_1", ());
}

UNIT_TEST(IdentityStore_ReportNicknamePostsJsonWithoutFriendsHeaders)
{
  ScopedIdentitySettings scoped;
  IdentityStore::GrantCompetitionConsent();
  backend::SetApiBaseUrl("https://example.com/api");
  int posts = 0;
  std::string lastUrl;
  std::string lastBody;
  std::vector<std::pair<std::string, std::string>> lastHeaders;
  IdentityStore::SetNicknameClaimPostFnForTesting(
      [&](std::string const & url, std::string const & body,
          std::vector<std::pair<std::string, std::string>> const & headers)
      {
        ++posts;
        lastUrl = url;
        lastBody = body;
        lastHeaders = headers;
        return 200;
      });
  TEST(IdentityStore::ReportNickname("Bob_2", "hate") == IdentityStore::NicknameReportResult::Ok, ());
  TEST_EQUAL(posts, 1, ());
  TEST_EQUAL(lastUrl, backend::GetCompetitionReportUrl(), ());
  TEST(IdentityJsonHasQuotedKey(lastBody, "profile_id"), (lastBody));
  TEST(IdentityJsonHasQuotedKey(lastBody, "target_nickname"), (lastBody));
  TEST(IdentityJsonHasQuotedKey(lastBody, "reason"), (lastBody));
  TEST(lastBody.find("Bob_2") != std::string::npos, (lastBody));
  TEST(lastBody.find("hate") != std::string::npos, (lastBody));
  TEST(!HasForbiddenClaimHeader(lastHeaders), ());
  TEST(lastHeaders.empty(), ());
}
