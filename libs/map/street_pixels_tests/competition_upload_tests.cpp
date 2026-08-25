#include "testing/testing.hpp"

#include "map/competition_upload_payload.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace
{
bool JsonHasQuotedKey(std::string const & json, std::string_view key)
{
  std::string const needle = "\"" + std::string(key) + "\"";
  return json.find(needle) != std::string::npos;
}

std::vector<std::string> ExtractQuotedJsonKeys(std::string const & json)
{
  std::vector<std::string> keys;
  for (size_t i = 0; i + 1 < json.size(); ++i)
  {
    if (json[i] != '"')
      continue;
    size_t j = i + 1;
    while (j < json.size() && json[j] != '"')
    {
      if (json[j] == '\\' && j + 1 < json.size())
        j += 2;
      else
        ++j;
    }
    if (j >= json.size())
      break;
    size_t k = j + 1;
    while (k < json.size() && (json[k] == ' ' || json[k] == '\n' || json[k] == '\t'))
      ++k;
    if (k < json.size() && json[k] == ':')
      keys.emplace_back(json.substr(i + 1, j - i - 1));
    i = j;
  }
  return keys;
}

CompetitionUploadPayload SamplePayload()
{
  CompetitionUploadPayload payload;
  payload.m_profileId = "profile-sample";
  payload.m_nickname = "Alice_1";
  payload.m_mapDataVersion = 340;
  payload.m_scoreCalcVersion = 1;
  payload.m_lastUpdateUnix = 1770000000;
  CompetitionUploadArea area;
  area.m_areaOsmId = 10;
  area.m_ownershipScore = 100.0;
  area.m_liveCoveragePct = 100.0;
  area.m_eligible = true;
  payload.m_areas.push_back(area);
  CompetitionUploadWeeklyCity city;
  city.m_cityOsmId = 20;
  city.m_newLiveCount = 3;
  payload.m_weeklyCities.push_back(city);
  return payload;
}

std::set<std::string> const kAllowKeys = {
    "profile_id",         "nickname",         "map_data_version", "score_calc_version",
    "last_update_unix",   "areas",            "area_osm_id",      "ownership_score",
    "live_coverage_pct",  "eligible",         "weekly_cities",    "city_osm_id",
    "new_live_count"};

std::string_view constexpr kDenyKeys[] = {
    "lat",           "lon",         "latitude",          "longitude", "gps",        "track",
    "tracks",        "healpix",     "pixel_id",          "pixelId",   "deviceId",   "advertising",
    "gaid",          "idfa",        "aaid",              "friend",    "friends",    "user_id",
    "unique_live",   "uniqueLivePixels", "explored_pixels", "regionId", "weekStart", "speed",
    "bearing",       "accuracy",    "mercator",          "coord",     "live_movement", "session",
    "week_id"};
}  // namespace

UNIT_TEST(CompetitionUpload_PayloadAllowList)
{
  std::string const json = SerializeCompetitionUploadPayload(SamplePayload());
  auto const keys = ExtractQuotedJsonKeys(json);
  TEST(!keys.empty(), ());
  std::set<std::string> seen;
  for (auto const & key : keys)
  {
    TEST(kAllowKeys.count(key) == 1, (key, json));
    seen.insert(key);
  }
  for (auto const & allowed : kAllowKeys)
    TEST(seen.count(allowed) == 1, (allowed, json));
  TEST_EQUAL(json.find("uniqueLivePixels"), std::string::npos, (json));
  TEST_EQUAL(json.find("unique_live"), std::string::npos, (json));
}

UNIT_TEST(CompetitionUpload_PayloadDenyList)
{
  std::string const json = SerializeCompetitionUploadPayload(SamplePayload());
  TEST(JsonHasQuotedKey(json, "last_update_unix"), (json));
  TEST(!JsonHasQuotedKey(json, "lat"), (json));
  TEST(!JsonHasQuotedKey(json, "lon"), (json));
  for (auto const key : kDenyKeys)
    TEST(!JsonHasQuotedKey(json, key), (key, json));
}
