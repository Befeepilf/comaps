#include "map/identity_store.hpp"

#include "map/backend_config.hpp"

#include "platform/http_client.hpp"
#include "platform/secure_storage.hpp"
#include "platform/settings.hpp"

#include "base/string_utils.hpp"
#include "base/timer.hpp"
#include "base/visitor.hpp"

#include "coding/base64.hpp"
#include "coding/serdes_json.hpp"
#include "coding/writer.hpp"

#include <utf8.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
constexpr char kDeviceIdKey[] = "Explore.DeviceId";
constexpr char kUsernameKey[] = "Explore.Username";
constexpr char kExploreConsentKey[] = "Explore.ConsentGiven";
constexpr char kCompetitionEnabledKey[] = "Explore.CompetitionEnabled";
constexpr char kAggregateSharingEnabledKey[] = "Explore.AggregateSharingEnabled";
constexpr char kConsentPolicyVersionKey[] = "Explore.ConsentPolicyVersion";
constexpr char kConsentUnixTimeKey[] = "Explore.ConsentUnixTime";
constexpr char kNicknameDraftKey[] = "Explore.NicknameDraft";
constexpr char kNicknameLastChangedKey[] = "Explore.NicknameLastChangedUnix";

IdentityStore::NicknameClaimHandler & ClaimHandler()
{
  static IdentityStore::NicknameClaimHandler handler;
  return handler;
}

std::mutex & ClaimHandlerMutex()
{
  static std::mutex mutex;
  return mutex;
}

IdentityStore::NicknameClaimPostFn & ClaimPostFn()
{
  static IdentityStore::NicknameClaimPostFn fn;
  return fn;
}

std::mutex & ClaimPostMutex()
{
  static std::mutex mutex;
  return mutex;
}

int DefaultNicknameClaimPost(std::string const & url, std::string const & body,
                             std::vector<std::pair<std::string, std::string>> const & headers)
{
  platform::HttpClient req(url);
  req.SetBodyData(body, "application/json");
  for (auto const & header : headers)
    req.SetRawHeader(header.first, header.second);
  std::string response;
  bool const ok = req.RunHttpRequest(response);
  if (!ok && req.ErrorCode() == platform::HttpClient::kNoError)
    return 0;
  return req.ErrorCode();
}

int PostCompetitionJson(std::string const & url, std::string const & json)
{
  if (url.empty())
    return 0;
  IdentityStore::NicknameClaimPostFn post;
  {
    std::lock_guard<std::mutex> lock(ClaimPostMutex());
    post = ClaimPostFn();
  }
  if (!post)
    post = &DefaultNicknameClaimPost;
  return post(url, json, {});
}

bool IsAllowedReportReason(std::string_view reason)
{
  return reason == "hate" || reason == "threats" || reason == "harassment" ||
         reason == "impersonation" || reason == "spam" || reason == "other";
}

struct NicknameClaimBody
{
  std::string m_profileId;
  std::string m_nickname;
  std::string m_consentPolicyVersion;
  uint64_t m_consentUnix = 0;

  DECLARE_VISITOR(visitor(m_profileId, "profile_id"), visitor(m_nickname, "nickname"),
                  visitor(m_consentPolicyVersion, "consent_policy_version"), visitor(m_consentUnix, "consent_unix"))
};

struct ProfileIdBody
{
  std::string m_profileId;

  DECLARE_VISITOR(visitor(m_profileId, "profile_id"))
};

struct NicknameReportBody
{
  std::string m_profileId;
  std::string m_targetNickname;
  std::string m_reason;

  DECLARE_VISITOR(visitor(m_profileId, "profile_id"), visitor(m_targetNickname, "target_nickname"),
                  visitor(m_reason, "reason"))
};

IdentityStore::CompetitionConsentGrantedHandler & ConsentGrantedHandler()
{
  static IdentityStore::CompetitionConsentGrantedHandler handler;
  return handler;
}

std::mutex & ConsentGrantedHandlerMutex()
{
  static std::mutex mutex;
  return mutex;
}

std::string ToUrlSafeBase64(std::string s)
{
  for (char & ch : s)
  {
    if (ch == '+')
      ch = '-';
    else if (ch == '/')
      ch = '_';
  }
  while (!s.empty() && s.back() == '=')
    s.pop_back();
  return s;
}

bool DecodeCodepoints(std::string_view s, std::vector<char32_t> & out)
{
  if (!utf8::is_valid(s.begin(), s.end()))
    return false;
  out.clear();
  auto it = s.begin();
  auto const end = s.end();
  while (it != end)
    out.push_back(utf8::unchecked::next(it));
  return true;
}

std::string EncodeCodepoints(std::vector<char32_t> const & cps)
{
  std::string out;
  for (char32_t const cp : cps)
    utf8::unchecked::append(static_cast<uint32_t>(cp), std::back_inserter(out));
  return out;
}

bool IsUnicodeSpace(char32_t c)
{
  return c == 0x00A0 || (c >= 0x2000 && c <= 0x200A) || c == 0x202F || c == 0x205F || c == 0x3000;
}

bool IsCombining(char32_t c)
{
  return (c >= 0x0300 && c <= 0x036F) || (c >= 0x1AB0 && c <= 0x1AFF) || (c >= 0x1DC0 && c <= 0x1DFF) ||
         (c >= 0x20D0 && c <= 0x20FF) || (c >= 0xFE20 && c <= 0xFE2F) || (c >= 0x0483 && c <= 0x0489) ||
         (c >= 0x0591 && c <= 0x05BD) || c == 0x05BF || (c >= 0x05C1 && c <= 0x05C2) ||
         (c >= 0x05C4 && c <= 0x05C5) || c == 0x05C7 || (c >= 0x0610 && c <= 0x061A) ||
         (c >= 0x064B && c <= 0x065F) || c == 0x0670 || (c >= 0x06D6 && c <= 0x06ED) ||
         (c >= 0x0900 && c <= 0x0903) || (c >= 0x093A && c <= 0x094D) || (c >= 0x0951 && c <= 0x0957) ||
         (c >= 0x0962 && c <= 0x0963) || c == 0x0E31 || (c >= 0x0E34 && c <= 0x0E3A) ||
         (c >= 0x0E47 && c <= 0x0E4E);
}

bool IsFormat(char32_t c)
{
  return c == 0x00AD || c == 0x180E || c == 0xFEFF || (c >= 0x200B && c <= 0x200F) ||
         (c >= 0x202A && c <= 0x202E) || (c >= 0x2060 && c <= 0x206F);
}

bool IsControl(char32_t c)
{
  return c <= 0x1F || (c >= 0x7F && c <= 0x9F) || c == 0x2028 || c == 0x2029;
}

bool IsLetter(char32_t c)
{
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
    return true;
  if ((c >= 0x00C0 && c <= 0x00D6) || (c >= 0x00D8 && c <= 0x00F6) || (c >= 0x00F8 && c <= 0x00FF))
    return true;
  if (c >= 0x0100 && c <= 0x024F)
    return true;
  if (c >= 0x1E00 && c <= 0x1EFF)
    return true;
  if (c >= 0x0370 && c <= 0x03FF)
    return true;
  if (c >= 0x1F00 && c <= 0x1FFF)
    return true;
  if (c >= 0x0400 && c <= 0x052F)
    return true;
  if (c >= 0x0530 && c <= 0x058F)
    return true;
  if (c >= 0x0590 && c <= 0x05FF)
    return true;
  if (c >= 0x0600 && c <= 0x06FF)
    return true;
  if (c >= 0x0900 && c <= 0x097F)
    return true;
  if (c >= 0x0E00 && c <= 0x0E7F)
    return true;
  if (c >= 0x10A0 && c <= 0x10FF)
    return true;
  if (c >= 0x1100 && c <= 0x11FF)
    return true;
  if (c >= 0x3040 && c <= 0x309F)
    return true;
  if (c >= 0x30A0 && c <= 0x30FF)
    return true;
  if (c >= 0x3130 && c <= 0x318F)
    return true;
  if (c >= 0x3400 && c <= 0x4DBF)
    return true;
  if (c >= 0x4E00 && c <= 0x9FFF)
    return true;
  if (c >= 0xAC00 && c <= 0xD7AF)
    return true;
  return false;
}

bool IsAllowedPunctuation(char32_t c)
{
  return c == ' ' || c == '_' || c == '-' || c == '.' || c == '\'' || c == 0x2019 || c == 0x00B7;
}

bool HasAsciiInsensitive(std::string const & s, std::string_view needle)
{
  std::string lower = s;
  strings::AsciiToLower(lower);
  return lower.find(needle) != std::string::npos;
}

bool HasEightConsecutiveAsciiDigits(std::string const & s)
{
  int run = 0;
  for (unsigned char const ch : s)
  {
    if (ch >= '0' && ch <= '9')
    {
      if (++run >= 8)
        return true;
    }
    else
      run = 0;
  }
  return false;
}

bool HasPhonePattern(std::string const & s)
{
  for (size_t i = 0; i < s.size(); ++i)
  {
    if (s[i] != '+')
      continue;
    int count = 0;
    for (size_t j = i + 1; j < s.size(); ++j)
    {
      char const c = s[j];
      if ((c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '(' || c == ')')
        ++count;
      else
        break;
    }
    if (count >= 8)
      return true;
  }
  return false;
}

bool HasTenOrMoreAsciiDigits(std::string const & s)
{
  int digits = 0;
  for (unsigned char const ch : s)
  {
    if (ch >= '0' && ch <= '9' && ++digits >= 10)
      return true;
  }
  return false;
}

bool HasUrlLikeTld(std::string const & s)
{
  std::string lower = s;
  strings::AsciiToLower(lower);
  static constexpr std::string_view kTlds[] = {".com", ".net", ".org", ".edu", ".gov", ".io",
                                               ".app", ".dev", ".xyz", ".info", ".biz", ".co"};
  for (auto const tld : kTlds)
  {
    auto pos = lower.find(tld);
    while (pos != std::string::npos)
    {
      if (pos > 0)
      {
        unsigned char const prev = static_cast<unsigned char>(lower[pos - 1]);
        if ((prev >= 'a' && prev <= 'z') || (prev >= '0' && prev <= '9'))
        {
          size_t const after = pos + tld.size();
          if (after == lower.size())
            return true;
          unsigned char const next = static_cast<unsigned char>(lower[after]);
          if (!((next >= 'a' && next <= 'z') || (next >= '0' && next <= '9')))
            return true;
        }
      }
      pos = lower.find(tld, pos + 1);
    }
  }
  return false;
}

uint32_t Fnv1a(std::string_view s)
{
  uint32_t hash = 2166136261u;
  for (unsigned char const c : s)
  {
    hash ^= c;
    hash *= 16777619u;
  }
  return hash;
}

std::string GetSettingString(char const * key)
{
  std::string value;
  if (settings::Get(std::string_view(key), value))
    return value;
  return {};
}

bool HasNonEmptySetting(char const * key)
{
  std::string value;
  return settings::Get(std::string_view(key), value) && !value.empty();
}
}  // namespace

void IdentityStore::MaybeMigrateLegacyUsername()
{
  std::string draft;
  uint64_t lastChanged = 0;
  bool const hasDraft = settings::Get(std::string_view(kNicknameDraftKey), draft) && !draft.empty();
  bool const hasLastChanged = settings::Get(std::string_view(kNicknameLastChangedKey), lastChanged);
  if (hasDraft || hasLastChanged)
    return;
  std::string username;
  if (!settings::Get(std::string_view(kUsernameKey), username) || username.empty())
    return;
  settings::Set(std::string_view(kNicknameDraftKey), username);
  settings::Delete(std::string_view(kUsernameKey));
}

std::string IdentityStore::GetOrCreateDeviceId()
{
  std::string deviceId;
  platform::SecureStorage storage;
  if (storage.Load(kDeviceIdKey, deviceId) && !deviceId.empty())
    return deviceId;

  deviceId = GenerateDeviceId();
  storage.Save(kDeviceIdKey, deviceId);
  return deviceId;
}

bool IdentityStore::HasUsername()
{
  MaybeMigrateLegacyUsername();
  return HasNonEmptySetting(kUsernameKey);
}

std::string IdentityStore::GetUsername()
{
  MaybeMigrateLegacyUsername();
  return GetSettingString(kUsernameKey);
}

bool IdentityStore::SetUsername(std::string_view username)
{
  return SetNicknameDraft(username);
}

void IdentityStore::ClearUsername()
{
  MaybeMigrateLegacyUsername();
  settings::Delete(std::string_view(kUsernameKey));
  settings::Delete(std::string_view(kNicknameDraftKey));
  settings::Delete(std::string_view(kNicknameLastChangedKey));
}

std::string IdentityStore::GetNicknameDraft()
{
  MaybeMigrateLegacyUsername();
  return GetSettingString(kNicknameDraftKey);
}

bool IdentityStore::SetNicknameDraft(std::string_view nickname)
{
  MaybeMigrateLegacyUsername();
  std::string const normalized = NormalizeNickname(nickname);
  if (!IsValidNickname(normalized))
    return false;
  settings::Set(std::string_view(kNicknameDraftKey), normalized);
  return true;
}

bool IdentityStore::HasExploreConsent()
{
  return HasCompetitionConsent();
}

bool IdentityStore::SetExploreConsent(bool consented)
{
  if (consented)
    GrantCompetitionConsent();
  else
    RevokeCompetitionConsent();
  return true;
}

bool IdentityStore::HasCompetitionConsent()
{
  bool competition = false;
  bool aggregate = false;
  std::string version;
  uint64_t unixTime = 0;
  if (!settings::Get(std::string_view(kCompetitionEnabledKey), competition) || !competition)
    return false;
  if (!settings::Get(std::string_view(kAggregateSharingEnabledKey), aggregate) || !aggregate)
    return false;
  if (!settings::Get(std::string_view(kConsentPolicyVersionKey), version) || version.empty() ||
      version != kCompetitionPrivacyPolicyVersion)
    return false;
  if (!settings::Get(std::string_view(kConsentUnixTimeKey), unixTime) || unixTime == 0)
    return false;
  return true;
}

uint64_t IdentityStore::GetCompetitionConsentUnixTime()
{
  uint64_t unixTime = 0;
  if (!settings::Get(std::string_view(kConsentUnixTimeKey), unixTime))
    return 0;
  return unixTime;
}

void IdentityStore::GrantCompetitionConsent()
{
  uint64_t const unixTime = base::SecondsSinceEpoch();
  settings::Set(std::string_view(kCompetitionEnabledKey), true);
  settings::Set(std::string_view(kAggregateSharingEnabledKey), true);
  settings::Set(std::string_view(kConsentPolicyVersionKey), std::string(kCompetitionPrivacyPolicyVersion));
  settings::Set(std::string_view(kConsentUnixTimeKey), unixTime);
  settings::Delete(std::string_view(kExploreConsentKey));

  CompetitionConsentGrantedHandler handler;
  {
    std::lock_guard<std::mutex> lock(ConsentGrantedHandlerMutex());
    handler = ConsentGrantedHandler();
  }
  if (handler)
    handler(unixTime);
}

void IdentityStore::RevokeCompetitionConsent()
{
  settings::Delete(std::string_view(kCompetitionEnabledKey));
  settings::Delete(std::string_view(kAggregateSharingEnabledKey));
  settings::Delete(std::string_view(kConsentPolicyVersionKey));
  settings::Delete(std::string_view(kConsentUnixTimeKey));
  settings::Delete(std::string_view(kExploreConsentKey));
}

bool IdentityStore::ShouldUploadCompetitionIdentity()
{
  return HasCompetitionConsent();
}

std::string IdentityStore::NormalizeNickname(std::string_view input)
{
  if (!utf8::is_valid(input.begin(), input.end()))
    return {};
  std::string s(input);
  strings::Trim(s);
  std::vector<char32_t> cps;
  if (!DecodeCodepoints(s, cps))
    return {};
  size_t begin = 0;
  size_t end = cps.size();
  while (begin < end && IsUnicodeSpace(cps[begin]))
    ++begin;
  while (end > begin && IsUnicodeSpace(cps[end - 1]))
    --end;
  std::vector<char32_t> out;
  bool lastSpace = false;
  for (size_t i = begin; i < end; ++i)
  {
    char32_t const c = cps[i];
    if (c == 0x20)
    {
      if (!lastSpace)
        out.push_back(0x20);
      lastSpace = true;
    }
    else
    {
      out.push_back(c);
      lastSpace = false;
    }
  }
  return EncodeCodepoints(out);
}

bool IdentityStore::IsValidNickname(std::string_view nickname)
{
  std::string const normalized = NormalizeNickname(nickname);
  if (normalized.empty())
    return false;
  if (!utf8::is_valid(normalized.begin(), normalized.end()))
    return false;
  std::vector<char32_t> cps;
  if (!DecodeCodepoints(normalized, cps))
    return false;

  int visible = 0;
  int combiningOnBase = 0;
  bool seenBase = false;
  for (char32_t const cp : cps)
  {
    if (IsControl(cp) || IsFormat(cp))
      return false;
    if (IsCombining(cp))
    {
      if (!seenBase)
        return false;
      ++combiningOnBase;
      if (combiningOnBase > 2)
        return false;
      continue;
    }
    combiningOnBase = 0;
    seenBase = true;
    if (!(IsLetter(cp) || (cp >= '0' && cp <= '9') || IsAllowedPunctuation(cp)))
      return false;
    ++visible;
  }
  if (!seenBase || visible < 3 || visible > 24)
    return false;
  if (HasAsciiInsensitive(normalized, "://") || HasAsciiInsensitive(normalized, "www.") ||
      HasAsciiInsensitive(normalized, "@") || HasUrlLikeTld(normalized))
    return false;
  if (HasEightConsecutiveAsciiDigits(normalized) || HasPhonePattern(normalized) ||
      HasTenOrMoreAsciiDigits(normalized))
    return false;
  if (normalized.find('/') != std::string::npos || normalized.find('\\') != std::string::npos ||
      normalized.find(':') != std::string::npos)
    return false;
  return true;
}

bool IdentityStore::IsValidUsername(std::string_view username)
{
  return IsValidNickname(username);
}

std::string IdentityStore::GenerateNickname()
{
  static uint32_t attempt = 0;
  return GenerateNickname(attempt++);
}

std::string IdentityStore::GenerateNickname(uint32_t attempt)
{
  static constexpr std::string_view kAdjectives[] = {
      "Amber", "Brave", "Calm", "Coral", "Crisp", "Eager", "Fair", "Fresh",
      "Gentle", "Happy", "Lucky", "Noble", "Proud", "Quiet", "Rapid", "Sunny"};
  static constexpr std::string_view kNouns[] = {
      "Cedar", "Falcon", "Garden", "Harbor", "Maple", "Meadow", "Ocean", "Pebble",
      "River", "Ridge", "Stone", "Trail", "Valley", "Willow", "Forest", "Canyon"};
  uint32_t const seed = Fnv1a(GetOrCreateDeviceId()) ^ attempt;
  std::mt19937 rng(seed);
  auto const adj = kAdjectives[rng() % std::size(kAdjectives)];
  auto const noun = kNouns[rng() % std::size(kNouns)];
  std::string out;
  out.append(adj);
  out.push_back(' ');
  out.append(noun);
  return out;
}

void IdentityStore::SetNicknameClaimHandlerForTesting(NicknameClaimHandler handler)
{
  std::lock_guard<std::mutex> lock(ClaimHandlerMutex());
  ClaimHandler() = std::move(handler);
}

void IdentityStore::SetNicknameClaimPostFnForTesting(NicknameClaimPostFn fn)
{
  std::lock_guard<std::mutex> lock(ClaimPostMutex());
  ClaimPostFn() = std::move(fn);
}

int IdentityStore::PostNicknameClaim(std::string_view nickname)
{
  bool const first = GetUsername().empty();
  std::string const url = first ? backend::GetCompetitionRegisterUrl() : backend::GetCompetitionNicknameUrl();
  if (url.empty())
    return 0;

  NicknameClaimBody body;
  body.m_profileId = GetOrCreateDeviceId();
  body.m_nickname = std::string(nickname);
  std::string version = GetSettingString(kConsentPolicyVersionKey);
  if (version.empty())
    version = std::string(kCompetitionPrivacyPolicyVersion);
  body.m_consentPolicyVersion = std::move(version);
  body.m_consentUnix = GetCompetitionConsentUnixTime();

  std::string json;
  {
    using Sink = MemWriter<std::string>;
    Sink sink(json);
    coding::SerializerJson<Sink> ser(sink);
    ser(body);
  }

  NicknameClaimPostFn post;
  {
    std::lock_guard<std::mutex> lock(ClaimPostMutex());
    post = ClaimPostFn();
  }
  if (!post)
    post = &DefaultNicknameClaimPost;
  return post(url, json, {});
}

void IdentityStore::SetCompetitionConsentGrantedHandler(CompetitionConsentGrantedHandler handler)
{
  std::lock_guard<std::mutex> lock(ConsentGrantedHandlerMutex());
  ConsentGrantedHandler() = std::move(handler);
}

IdentityStore::NicknameClaimResult IdentityStore::TryClaimNickname(std::string_view nickname)
{
  MaybeMigrateLegacyUsername();
  std::string const normalized = NormalizeNickname(nickname);
  if (!IsValidNickname(normalized))
    return NicknameClaimResult::Invalid;

  std::string const accepted = GetSettingString(kUsernameKey);
  if (!accepted.empty() && normalized == accepted)
    return NicknameClaimResult::Ok;

  if (!accepted.empty() && !CanRenameNickname())
    return NicknameClaimResult::RenameLimited;

  NicknameClaimHandler handler;
  {
    std::lock_guard<std::mutex> lock(ClaimHandlerMutex());
    handler = ClaimHandler();
  }
  if (!handler)
    handler = &IdentityStore::PostNicknameClaim;

  int const status = handler(normalized);
  if (status == 409)
  {
    settings::Set(std::string_view(kNicknameDraftKey), normalized);
    return NicknameClaimResult::Collision;
  }
  if (status == 200)
  {
    settings::Set(std::string_view(kNicknameLastChangedKey), base::SecondsSinceEpoch());
    settings::Set(std::string_view(kUsernameKey), normalized);
    settings::Delete(std::string_view(kNicknameDraftKey));
    return NicknameClaimResult::Ok;
  }
  settings::Set(std::string_view(kNicknameDraftKey), normalized);
  return NicknameClaimResult::Unavailable;
}

bool IdentityStore::CanRenameNickname()
{
  MaybeMigrateLegacyUsername();
  if (!HasNonEmptySetting(kUsernameKey))
    return true;
  uint64_t lastChanged = 0;
  if (!settings::Get(std::string_view(kNicknameLastChangedKey), lastChanged))
    return true;
  uint64_t const now = base::SecondsSinceEpoch();
  return now >= lastChanged && (now - lastChanged) >= kNicknameRenameIntervalSeconds;
}

IdentityStore::CompetitionAccountResult IdentityStore::LeaveCompetitionRetain()
{
  std::string const url = backend::GetCompetitionLeaveUrl();
  int status = 200;
  if (!url.empty())
  {
    ProfileIdBody body;
    body.m_profileId = GetOrCreateDeviceId();
    std::string json;
    {
      using Sink = MemWriter<std::string>;
      Sink sink(json);
      coding::SerializerJson<Sink> ser(sink);
      ser(body);
    }
    status = PostCompetitionJson(url, json);
  }
  RevokeCompetitionConsent();
  if (url.empty() || status == 200)
    return CompetitionAccountResult::Ok;
  return CompetitionAccountResult::Unavailable;
}

IdentityStore::CompetitionAccountResult IdentityStore::DeleteCompetitionProfile()
{
  std::string const url = backend::GetCompetitionDeleteUrl();
  if (url.empty())
    return CompetitionAccountResult::Unavailable;
  ProfileIdBody body;
  body.m_profileId = GetOrCreateDeviceId();
  std::string json;
  {
    using Sink = MemWriter<std::string>;
    Sink sink(json);
    coding::SerializerJson<Sink> ser(sink);
    ser(body);
  }
  int const status = PostCompetitionJson(url, json);
  if (status == 200 || status == 404)
  {
    RevokeCompetitionConsent();
    ClearUsername();
    return CompetitionAccountResult::Ok;
  }
  return CompetitionAccountResult::Unavailable;
}

IdentityStore::NicknameReportResult IdentityStore::ReportNickname(std::string_view target,
                                                                 std::string_view reason)
{
  std::string const normalized = NormalizeNickname(target);
  if (!IsValidNickname(normalized) || !IsAllowedReportReason(reason))
    return NicknameReportResult::Invalid;
  std::string const url = backend::GetCompetitionReportUrl();
  if (url.empty())
    return NicknameReportResult::Unavailable;
  NicknameReportBody body;
  body.m_profileId = GetOrCreateDeviceId();
  body.m_targetNickname = normalized;
  body.m_reason = std::string(reason);
  std::string json;
  {
    using Sink = MemWriter<std::string>;
    Sink sink(json);
    coding::SerializerJson<Sink> ser(sink);
    ser(body);
  }
  int const status = PostCompetitionJson(url, json);
  if (status == 200)
    return NicknameReportResult::Ok;
  if (status == 404)
    return NicknameReportResult::NotFound;
  return NicknameReportResult::Unavailable;
}

std::string IdentityStore::GenerateDeviceId()
{
  std::array<unsigned char, 24> bytes{};
  std::random_device rd;
  std::generate(bytes.begin(), bytes.end(), [&rd]() { return static_cast<unsigned char>(rd()); });

  std::string raw(reinterpret_cast<char const *>(bytes.data()), bytes.size());
  auto const b64 = base64::Encode(std::string_view(raw.data(), raw.size()));
  return ToUrlSafeBase64(b64);
}
