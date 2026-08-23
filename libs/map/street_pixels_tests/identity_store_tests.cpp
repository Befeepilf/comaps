#include "testing/testing.hpp"

#include "map/identity_store.hpp"

#include "platform/settings.hpp"

#include "base/timer.hpp"

#include <string>
#include <string_view>

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
  IdentityStore::SetNicknameClaimHandlerForTesting({});
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
  IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 409; });
  TEST(IdentityStore::TryClaimNickname("Bob_2") == IdentityStore::NicknameClaimResult::Collision, ());
  TEST_EQUAL(IdentityStore::GetUsername(), "Alice_1", ());
}

UNIT_TEST(IdentityStore_ConsentOffBlocksIdentityUpload)
{
  ScopedIdentitySettings scoped;
  TEST(!IdentityStore::ShouldUploadCompetitionIdentity(), ());
  IdentityStore::GrantCompetitionConsent();
  TEST(IdentityStore::HasCompetitionConsent(), ());
  TEST(IdentityStore::ShouldUploadCompetitionIdentity(), ());
  IdentityStore::RevokeCompetitionConsent();
  TEST(!IdentityStore::ShouldUploadCompetitionIdentity(), ());
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
