#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class IdentityStore
{
public:
  static constexpr std::string_view kCompetitionPrivacyPolicyVersion = "1";
  static constexpr uint64_t kNicknameRenameIntervalSeconds = 7 * 24 * 60 * 60;

  enum class NicknameClaimResult
  {
    Ok,
    Invalid,
    Collision,
    RenameLimited,
    Unavailable
  };

  using NicknameClaimHandler = std::function<int(std::string_view)>;
  using NicknameClaimPostFn =
      std::function<int(std::string const & url, std::string const & body,
                        std::vector<std::pair<std::string, std::string>> const & headers)>;
  using CompetitionConsentGrantedHandler = std::function<void(uint64_t)>;

  static std::string GetOrCreateDeviceId();

  static bool HasUsername();
  static std::string GetUsername();
  static bool SetUsername(std::string_view username);
  static void ClearUsername();

  static std::string GetNicknameDraft();
  static bool SetNicknameDraft(std::string_view nickname);

  static bool HasExploreConsent();
  static bool SetExploreConsent(bool consented);

  static bool HasCompetitionConsent();
  static uint64_t GetCompetitionConsentUnixTime();
  static void GrantCompetitionConsent();
  static void RevokeCompetitionConsent();
  static bool ShouldUploadCompetitionIdentity();
  static void SetCompetitionConsentGrantedHandler(CompetitionConsentGrantedHandler handler);

  static std::string NormalizeNickname(std::string_view input);
  static bool IsValidNickname(std::string_view nickname);
  static bool IsValidUsername(std::string_view username);

  static std::string GenerateNickname();
  static std::string GenerateNickname(uint32_t attempt);

  static void SetNicknameClaimHandlerForTesting(NicknameClaimHandler handler);
  static void SetNicknameClaimPostFnForTesting(NicknameClaimPostFn fn);
  static int PostNicknameClaim(std::string_view nickname);
  static NicknameClaimResult TryClaimNickname(std::string_view nickname);
  static bool CanRenameNickname();

private:
  static std::string GenerateDeviceId();
  static void MaybeMigrateLegacyUsername();
};
