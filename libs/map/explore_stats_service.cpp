#include "map/explore_stats_service.hpp"

#include "map/backend_config.hpp"
#include "map/identity_store.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/serdes_json.hpp"
#include "coding/writer.hpp"

#include "base/logging.hpp"

#include <chrono>

namespace
{
constexpr char kStatsFile[] = "explore_stats.json";
constexpr char kSyncEnabledKey[] = "Explore.SyncEnabled";
constexpr char kFriendVisibilityEnabledKey[] = "Explore.FriendVisibilityEnabled";
constexpr char kLegacySharingEnabledKey[] = "Explore.SharingEnabled";
}  // namespace

ExploreStatsService::ExploreStatsService()
{
  bool syncEnabled = false;
  bool friendVisibilityEnabled = false;
  bool legacySharingEnabled = false;
  if (settings::Get(kSyncEnabledKey, syncEnabled) || settings::Get(kFriendVisibilityEnabledKey, friendVisibilityEnabled))
  {
    // New keys already present.
  }
  else if (settings::Get(kLegacySharingEnabledKey, legacySharingEnabled) && legacySharingEnabled)
  {
    syncEnabled = true;
    friendVisibilityEnabled = true;
    settings::Set(kSyncEnabledKey, true);
    settings::Set(kFriendVisibilityEnabledKey, true);
  }

  std::lock_guard<std::mutex> lock(m_mutex);
  m_syncEnabled = syncEnabled;
  m_friendVisibilityEnabled = friendVisibilityEnabled;
  SchedulePeriodicUpload();
}

void ExploreStatsService::EnableSync(bool enabled)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_syncEnabled = enabled;
  settings::Set(kSyncEnabledKey, enabled);
}

bool ExploreStatsService::IsSyncEnabled() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_syncEnabled;
}

void ExploreStatsService::EnableFriendVisibility(bool enabled)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_friendVisibilityEnabled = enabled;
  settings::Set(kFriendVisibilityEnabledKey, enabled);
}

bool ExploreStatsService::IsFriendVisibilityEnabled() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_friendVisibilityEnabled;
}

void ExploreStatsService::EnableSharing(bool enabled)
{
  EnableSync(enabled);
  EnableFriendVisibility(enabled);
}

bool ExploreStatsService::IsSharingEnabled() const
{
  return IsSyncEnabled() && IsFriendVisibilityEnabled();
}

void ExploreStatsService::GetEntries(std::vector<StatsEntry> & out) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  out.clear();
  out.reserve(m_keyToEntry.size());
  for (auto const & kv : m_keyToEntry)
    out.push_back(kv.second);
}

void ExploreStatsService::ResetRegion(std::string const & regionId)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto it = m_keyToEntry.begin(); it != m_keyToEntry.end();)
  {
    if (it->second.m_regionId == regionId)
      it = m_keyToEntry.erase(it);
    else
      ++it;
  }
  ScheduleSave();
}

uint64_t ExploreStatsService::ToWeekBucket(uint64_t s)
{
  // 7 days per bucket
  uint64_t constexpr kWeek = 7ull * 24ull * 60ull * 60ull;
  return (s / kWeek) * kWeek;
}

std::string ExploreStatsService::MakeKey(std::string const & regionId, uint64_t weekStartSec)
{
  return regionId + "|" + std::to_string(weekStartSec);
}

std::string ExploreStatsService::GetFilePath() const { return GetPlatform().WritablePathForFile(kStatsFile); }

void ExploreStatsService::EnsureLoaded()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_loaded)
    return;

  std::string const file = GetFilePath();
  if (!Platform::IsFileExistsByFullPath(file))
  {
    m_loaded = true;
    return;
  }

  try
  {
    std::string json;
    FileReader reader(file);
    uint64_t const size = reader.Size();
    json.resize(static_cast<size_t>(size));
    reader.Read(0, json.data(), json.size());

    Snapshot snap;
    coding::DeserializerJson des(json);
    des(snap);
    m_keyToEntry.clear();
    for (auto const & e : snap.m_entries)
      m_keyToEntry.emplace(MakeKey(e.m_regionId, e.m_weekStartSec), e);
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("Failed to load stats:", e.what()));
  }
  m_loaded = true;
}

void ExploreStatsService::OnExplorationDelta(std::string const & regionId, uint32_t delta, double eventTimeSeconds)
{
  if (delta == 0)
    return;

  EnsureLoaded();

  uint64_t const ts = static_cast<uint64_t>(eventTimeSeconds);
  uint64_t const week = ToWeekBucket(ts);
  std::string const key = MakeKey(regionId, week);

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto & entry = m_keyToEntry[key];
    if (entry.m_regionId.empty())
    {
      entry.m_regionId = regionId;
      entry.m_weekStartSec = week;
    }
    entry.m_exploredPixels += delta;
    entry.m_version += 1;
    m_changedAt = std::chrono::steady_clock::now();
  }

  ScheduleSave();
}

void ExploreStatsService::ScheduleSave()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_saveScheduled)
    return;
  m_saveScheduled = true;
  GetPlatform().RunDelayedTask(Platform::Thread::Background, std::chrono::seconds(2), [this]() { Save(); });
}

void ExploreStatsService::Save()
{
  Snapshot snap;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    snap.m_entries.reserve(m_keyToEntry.size());
    for (auto const & kv : m_keyToEntry)
      snap.m_entries.push_back(kv.second);
    m_saveScheduled = false;
  }

  try
  {
    auto const path = GetFilePath();
    FileWriter writer(path);
    coding::SerializerJson<FileWriter> ser(writer);
    ser(snap);
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("Failed to save stats:", e.what()));
  }
}

void ExploreStatsService::SchedulePeriodicUpload()
{
  GetPlatform().RunDelayedTask(Platform::Thread::Background, std::chrono::minutes(1),
                               [this]()
                               {
                                 LOG(LINFO, ("Checking if new stats need to be uploaded"));
                                 bool shouldUpload = false;
                                 {
                                   std::lock_guard<std::mutex> lock(m_mutex);
                                   shouldUpload = m_changedAt > m_lastUploadAt;
                                 }
                                 if (shouldUpload)
                                   TryUpload();
                                 else
                                   LOG(LINFO, ("No new stats to upload"));
                                 SchedulePeriodicUpload();
                               });
}

void ExploreStatsService::TryUpload()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_syncEnabled)
    {
      LOG(LINFO, ("Sync disabled; skipping upload"));
      return;
    }
  }

  if (!backend::IsApiConfigured())
  {
    LOG(LINFO, ("API base not configured; skipping upload"));
    return;
  }

  std::string const url = backend::GetStatsUploadUrl();
  if (url.empty())
    return;

  LOG(LINFO, ("Uploading stats..."));

  std::string const body = BuildUploadJson();

  GetPlatform().RunTask(Platform::Thread::Network,
                        [this, body, url]()
                        {
                          platform::HttpClient req(url);
                          req.SetBodyData(body, "application/json");
                          std::string response;
                          bool const ok = req.RunHttpRequest(response);
                          if (!ok || req.ErrorCode() != 200)
                          {
                            LOG(LWARNING, ("Stats upload failed:", req.ErrorCode()));
                            return;
                          }
                          LOG(LINFO, ("Stats uploaded"));
                          {
                            std::lock_guard<std::mutex> lock(m_mutex);
                            m_lastUploadAt = std::chrono::steady_clock::now();
                          }
                        });
}

std::string ExploreStatsService::BuildUploadJson() const
{
  UploadPayload payload;
  payload.m_deviceId = IdentityStore::GetOrCreateDeviceId();
  if (IdentityStore::HasUsername())
    payload.m_username = IdentityStore::GetUsername();

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    payload.m_entries.reserve(m_keyToEntry.size());
    for (auto const & kv : m_keyToEntry)
      payload.m_entries.push_back(kv.second);
  }

  std::string jsonStr;
  using Sink = MemWriter<std::string>;
  Sink sink(jsonStr);
  coding::SerializerJson<Sink> ser(sink);
  ser(payload);
  return jsonStr;
}