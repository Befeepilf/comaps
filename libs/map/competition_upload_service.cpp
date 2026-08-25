#include "map/competition_upload_service.hpp"

#include "map/backend_config.hpp"
#include "map/identity_store.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "base/scope_guard.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <random>
#include <string>
#include <string_view>

namespace
{
constexpr char kPendingKey[] = "Explore.CompetitionUploadPending";
constexpr char kLastSuccessKey[] = "Explore.CompetitionLastSuccessUnix";
constexpr char kNextAllowedKey[] = "Explore.CompetitionNextAllowedUnix";

int64_t DefaultNow() { return static_cast<int64_t>(base::SecondsSinceEpoch()); }

int64_t DefaultJitter()
{
  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_int_distribution<int64_t> dist(0, kCompetitionMaxJitterSeconds);
  return dist(rng);
}

bool DefaultConnected() { return Platform::IsConnected(); }

int DefaultPost(std::string const & url, std::string const & body, CompetitionUploadService::Headers const & headers)
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

uint64_t LoadUint64(char const * key)
{
  uint64_t value = 0;
  if (!settings::Get(std::string_view(key), value))
    return 0;
  return value;
}
}  // namespace

CompetitionUploadService::CompetitionUploadService() : CompetitionUploadService({}, {}, {}, {}, {}) {}

CompetitionUploadService::CompetitionUploadService(NowFn nowFn, JitterFn jitterFn, ConnectedFn connectedFn,
                                                   PostFn postFn, SnapshotFn snapshotFn)
  : m_nowFn(nowFn ? std::move(nowFn) : NowFn(&DefaultNow))
  , m_jitterFn(jitterFn ? std::move(jitterFn) : JitterFn(&DefaultJitter))
  , m_connectedFn(connectedFn ? std::move(connectedFn) : ConnectedFn(&DefaultConnected))
  , m_postFn(postFn ? std::move(postFn) : PostFn(&DefaultPost))
  , m_snapshotFn(std::move(snapshotFn))
{}

void CompetitionUploadService::SetSnapshotFn(SnapshotFn snapshotFn)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_snapshotFn = std::move(snapshotFn);
}

int64_t CompetitionUploadService::ClampedJitter() const
{
  int64_t jitter = m_jitterFn ? m_jitterFn() : 0;
  if (jitter < 0)
    jitter = 0;
  if (jitter > kCompetitionMaxJitterSeconds)
    jitter = kCompetitionMaxJitterSeconds;
  return jitter;
}

uint64_t CompetitionUploadService::LoadNextAllowedUnlocked() const { return LoadUint64(kNextAllowedKey); }

bool CompetitionUploadService::LoadPendingUnlocked() const
{
  bool pending = false;
  if (!settings::Get(std::string_view(kPendingKey), pending))
    return false;
  return pending;
}

void CompetitionUploadService::MarkPending()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  settings::Set(std::string_view(kPendingKey), true);
  if (m_uploadInFlight)
    m_markedWhileInFlight = true;
  if (LoadNextAllowedUnlocked() != 0)
    return;
  int64_t const now = m_nowFn();
  uint64_t const nextAllowed =
      static_cast<uint64_t>(now + kCompetitionMinUploadIntervalSeconds + ClampedJitter());
  settings::Set(std::string_view(kNextAllowedKey), nextAllowed);
}

void CompetitionUploadService::MaybeUpload()
{
  std::string url;
  SnapshotFn snapshotFn;
  PostFn postFn;
  int64_t gateNow = 0;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!IdentityStore::HasCompetitionConsent())
    {
      settings::Set(std::string_view(kPendingKey), false);
      m_markedWhileInFlight = false;
      return;
    }
    if (!IdentityStore::HasUsername())
      return;
    if (backend::GetApiBaseUrl().empty())
      return;
    url = backend::GetCompetitionAggregatesUrl();
    if (url.empty())
      return;
    if (!LoadPendingUnlocked())
      return;
    gateNow = m_nowFn();
    if (gateNow < static_cast<int64_t>(LoadNextAllowedUnlocked()))
      return;
    if (!m_connectedFn())
      return;
    if (m_uploadInFlight)
      return;
    m_uploadInFlight = true;
    snapshotFn = m_snapshotFn;
    postFn = m_postFn;
  }

  SCOPE_GUARD(clearInFlight, [this]()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_uploadInFlight = false;
  });

  int64_t const now = m_nowFn();
  CompetitionUploadPayload payload = snapshotFn ? snapshotFn(now) : CompetitionUploadPayload{};
  if (CompetitionUploadPayloadIsEmpty(payload))
    return;

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!IdentityStore::HasCompetitionConsent())
    {
      settings::Set(std::string_view(kPendingKey), false);
      m_markedWhileInFlight = false;
      return;
    }
    if (!IdentityStore::HasUsername())
      return;
    if (backend::GetApiBaseUrl().empty())
      return;
    url = backend::GetCompetitionAggregatesUrl();
    if (url.empty())
      return;
    std::string const nickname = IdentityStore::GetUsername();
    if (nickname.empty())
      return;
    payload.m_profileId = IdentityStore::GetOrCreateDeviceId();
    payload.m_nickname = nickname;
  }

  if (!IdentityStore::HasCompetitionConsent() || backend::GetApiBaseUrl().empty())
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!IdentityStore::HasCompetitionConsent())
    {
      settings::Set(std::string_view(kPendingKey), false);
      m_markedWhileInFlight = false;
    }
    return;
  }

  std::string const body = SerializeCompetitionUploadPayload(payload);
  Headers headers;
  int const status = postFn ? postFn(url, body, headers) : 0;

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    int64_t const cadenceNow = std::max(gateNow, now);
    uint64_t const nextAllowed =
        static_cast<uint64_t>(cadenceNow + kCompetitionMinUploadIntervalSeconds + ClampedJitter());
    settings::Set(std::string_view(kNextAllowedKey), nextAllowed);
    bool const keepPending = status != 200 || m_markedWhileInFlight;
    m_markedWhileInFlight = false;
    if (keepPending)
      return;
    settings::Set(std::string_view(kPendingKey), false);
    settings::Set(std::string_view(kLastSuccessKey), static_cast<uint64_t>(cadenceNow));
  }
}
