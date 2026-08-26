#include "map/competition_snapshot.hpp"

#include "map/backend_config.hpp"

#include "cppjansson/cppjansson.hpp"

#include "platform/http_client.hpp"
#include "platform/settings.hpp"

#include <mutex>
#include <utility>

namespace street_pixels
{
namespace
{
constexpr char kMapModeKey[] = "StreetPixels.CompetitionMapMode";
constexpr char kOvertakingLastUnixKey[] = "StreetPixels.OvertakingHintLastUnix";

street_pixels::CompetitionGetFn & GetFn()
{
  static street_pixels::CompetitionGetFn fn;
  return fn;
}

std::mutex & GetFnMutex()
{
  static std::mutex mutex;
  return mutex;
}

std::optional<CompetitionAreaSnapshot> & SnapshotCache()
{
  static std::optional<CompetitionAreaSnapshot> cache;
  return cache;
}

std::mutex & SnapshotCacheMutex()
{
  static std::mutex mutex;
  return mutex;
}

int DefaultCompetitionGet(std::string const & url, std::string & response,
                          std::vector<std::pair<std::string, std::string>> const & headers)
{
  platform::HttpClient req(url);
  req.SetHttpMethod("GET");
  for (auto const & header : headers)
    req.SetRawHeader(header.first, header.second);
  bool const ok = req.RunHttpRequest(response);
  if (!ok && req.ErrorCode() == platform::HttpClient::kNoError)
    return 0;
  if (ok)
    return 200;
  return req.ErrorCode();
}

std::optional<std::string> NicknameFromJson(json_t * obj)
{
  json_t * field = base::GetJSONOptionalField(obj, "nickname");
  if (!field || json_is_null(field))
    return std::nullopt;
  if (!json_is_string(field))
    return std::nullopt;
  return std::string(json_string_value(field));
}

bool ParseRankingEntry(json_t * item, CompetitionRankingEntry & entry)
{
  if (!item || !json_is_object(item))
    return false;
  FromJSONObject(item, "rank", entry.m_rank);
  entry.m_nickname = NicknameFromJson(item);
  FromJSONObject(item, "decayed_score", entry.m_decayedScore);
  FromJSONObject(item, "gap_to_leader", entry.m_gapToLeader);
  json_t * gap = base::GetJSONOptionalField(item, "gap_to_current_user");
  if (gap && !json_is_null(gap))
  {
    double value = 0.0;
    FromJSON(gap, value);
    entry.m_gapToCurrentUser = value;
  }
  else
  {
    entry.m_gapToCurrentUser = std::nullopt;
  }
  FromJSONObject(item, "is_current_user", entry.m_isCurrentUser);
  FromJSONObject(item, "eligible", entry.m_eligible);
  return true;
}

bool ParseWeeklyEntry(json_t * item, CompetitionWeeklyEntry & entry)
{
  if (!item || !json_is_object(item))
    return false;
  FromJSONObject(item, "rank", entry.m_rank);
  entry.m_nickname = NicknameFromJson(item);
  FromJSONObject(item, "new_live_count", entry.m_newLiveCount);
  FromJSONObject(item, "gap_to_leader", entry.m_gapToLeader);
  json_t * gap = base::GetJSONOptionalField(item, "gap_to_current_user");
  if (gap && !json_is_null(gap))
  {
    double value = 0.0;
    FromJSON(gap, value);
    entry.m_gapToCurrentUser = value;
  }
  else
  {
    entry.m_gapToCurrentUser = std::nullopt;
  }
  FromJSONObject(item, "is_current_user", entry.m_isCurrentUser);
  return true;
}

bool ParseBoss(json_t * bossObj, CompetitionBoss & boss)
{
  if (!bossObj || !json_is_object(bossObj))
    return false;
  boss.m_nickname = NicknameFromJson(bossObj);
  FromJSONObject(bossObj, "decayed_score", boss.m_decayedScore);
  FromJSONObject(bossObj, "is_current_user", boss.m_isCurrentUser);
  return true;
}

FetchAreaSnapshotResult ChromeFromCacheOrOffline(int64_t areaOsmId)
{
  FetchAreaSnapshotResult result;
  auto last = LastAreaSnapshot();
  if (last.has_value() && last->m_areaOsmId == areaOsmId)
  {
    result.m_snapshot = last;
    result.m_chrome = BuildCompetitionAreaChrome(last);
    result.m_chrome.m_offline = true;
    result.m_chrome.m_stale = true;
  }
  else
  {
    result.m_chrome.m_offline = true;
  }
  return result;
}
}  // namespace

bool ParseAreaSnapshotJson(std::string const & json, CompetitionAreaSnapshot & out)
{
  try
  {
    base::Json root(json);
    if (!root.get() || !json_is_object(root.get()))
      return false;
    CompetitionAreaSnapshot parsed;
    json_t * profile = base::GetJSONOptionalField(root.get(), "profile_id");
    if (profile && json_is_string(profile))
      parsed.m_profileId = json_string_value(profile);
    json_t * osm = base::GetJSONOptionalField(root.get(), "area_osm_id");
    if (osm && json_is_number(osm))
      parsed.m_areaOsmId = static_cast<int64_t>(json_integer_value(osm));
    FromJSONObject(root.get(), "unclaimed", parsed.m_unclaimed);
    FromJSONObject(root.get(), "contested", parsed.m_contested);
    FromJSONObject(root.get(), "stale", parsed.m_stale);
    FromJSONObject(root.get(), "participant_count", parsed.m_participantCount);
    json_t * boss = base::GetJSONObligatoryField(root.get(), "boss");
    if (json_is_null(boss))
      parsed.m_boss = std::nullopt;
    else
    {
      CompetitionBoss parsedBoss;
      if (!ParseBoss(boss, parsedBoss))
        return false;
      parsed.m_boss = parsedBoss;
    }
    json_t * ranking = base::GetJSONObligatoryField(root.get(), "ranking");
    if (!json_is_array(ranking))
      return false;
    size_t const n = json_array_size(ranking);
    parsed.m_ranking.reserve(n);
    for (size_t i = 0; i < n; ++i)
    {
      CompetitionRankingEntry entry;
      if (!ParseRankingEntry(json_array_get(ranking, i), entry))
        return false;
      parsed.m_ranking.push_back(std::move(entry));
    }
    out = std::move(parsed);
    return true;
  }
  catch (base::Json::Exception const &)
  {
    return false;
  }
}

bool ParseWeeklyBoardJson(std::string const & json, CompetitionWeeklyBoard & out)
{
  try
  {
    base::Json root(json);
    if (!root.get() || !json_is_object(root.get()))
      return false;
    CompetitionWeeklyBoard parsed;
    json_t * profile = base::GetJSONOptionalField(root.get(), "profile_id");
    if (profile && json_is_string(profile))
      parsed.m_profileId = json_string_value(profile);
    json_t * city = base::GetJSONOptionalField(root.get(), "city_osm_id");
    if (city && json_is_number(city))
      parsed.m_cityOsmId = static_cast<int64_t>(json_integer_value(city));
    json_t * week = base::GetJSONOptionalField(root.get(), "week_start_unix");
    if (week && json_is_number(week))
      parsed.m_weekStartUnix = static_cast<int64_t>(json_integer_value(week));
    json_t * remaining = base::GetJSONOptionalField(root.get(), "seconds_remaining");
    if (remaining && json_is_number(remaining))
      parsed.m_secondsRemaining = static_cast<int64_t>(json_integer_value(remaining));
    FromJSONObject(root.get(), "participant_count", parsed.m_participantCount);
    json_t * ranking = base::GetJSONObligatoryField(root.get(), "ranking");
    if (!json_is_array(ranking))
      return false;
    size_t const n = json_array_size(ranking);
    parsed.m_ranking.reserve(n);
    for (size_t i = 0; i < n; ++i)
    {
      CompetitionWeeklyEntry entry;
      if (!ParseWeeklyEntry(json_array_get(ranking, i), entry))
        return false;
      parsed.m_ranking.push_back(std::move(entry));
    }
    out = std::move(parsed);
    return true;
  }
  catch (base::Json::Exception const &)
  {
    return false;
  }
}

void SetCompetitionGetFnForTesting(CompetitionGetFn fn)
{
  std::lock_guard<std::mutex> lock(GetFnMutex());
  GetFn() = std::move(fn);
}

int GetCompetitionJson(std::string const & url, std::string & response)
{
  CompetitionGetFn fn;
  {
    std::lock_guard<std::mutex> lock(GetFnMutex());
    fn = GetFn();
  }
  if (!fn)
    fn = &DefaultCompetitionGet;
  return fn(url, response, {});
}

CompetitionMapMode GetCompetitionMapMode()
{
  uint64_t value = 0;
  settings::TryGet(kMapModeKey, value);
  if (value == static_cast<uint64_t>(CompetitionMapMode::Competition))
    return CompetitionMapMode::Competition;
  return CompetitionMapMode::Explore;
}

void SetCompetitionMapMode(CompetitionMapMode mode)
{
  settings::Set(kMapModeKey, static_cast<uint64_t>(mode));
}

std::optional<CompetitionAreaSnapshot> LastAreaSnapshot()
{
  std::lock_guard<std::mutex> lock(SnapshotCacheMutex());
  return SnapshotCache();
}

void ClearCompetitionSnapshotCacheForTesting()
{
  std::lock_guard<std::mutex> lock(SnapshotCacheMutex());
  SnapshotCache().reset();
}

FetchAreaSnapshotResult FetchAreaSnapshot(int64_t areaOsmId, std::string const & profileId)
{
  FetchAreaSnapshotResult result;
  result.m_url = backend::GetCompetitionAreaSnapshotRequestUrl(areaOsmId, profileId);
  if (result.m_url.empty())
    return ChromeFromCacheOrOffline(areaOsmId);

  std::string body;
  result.m_didGet = true;
  result.m_httpStatus = GetCompetitionJson(result.m_url, body);
  if (result.m_httpStatus != 200)
  {
    auto cached = ChromeFromCacheOrOffline(areaOsmId);
    cached.m_didGet = true;
    cached.m_httpStatus = result.m_httpStatus;
    cached.m_url = result.m_url;
    return cached;
  }

  CompetitionAreaSnapshot parsed;
  if (!ParseAreaSnapshotJson(body, parsed))
  {
    auto cached = ChromeFromCacheOrOffline(areaOsmId);
    cached.m_didGet = true;
    cached.m_httpStatus = result.m_httpStatus;
    cached.m_url = result.m_url;
    return cached;
  }

  parsed.m_areaOsmId = areaOsmId;
  {
    std::lock_guard<std::mutex> lock(SnapshotCacheMutex());
    SnapshotCache() = parsed;
  }
  result.m_snapshot = parsed;
  result.m_chrome = BuildCompetitionAreaChrome(parsed);
  return result;
}

bool ShouldEmitOvertakingHint(uint64_t nowUnix)
{
  uint64_t last = 0;
  settings::TryGet(kOvertakingLastUnixKey, last);
  if (last == 0)
    return true;
  if (nowUnix < last)
    return true;
  return (nowUnix - last) >= kOvertakingHintMinIntervalSeconds;
}

void MarkOvertakingHintEmitted(uint64_t nowUnix)
{
  settings::Set(kOvertakingLastUnixKey, nowUnix);
}

void ClearOvertakingHintForTesting()
{
  settings::Delete(kOvertakingLastUnixKey);
}
}  // namespace street_pixels
