#include "testing/testing.hpp"

#include "map/backend_config.hpp"
#include "map/competition_snapshot.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{
char const kNullNicknameJson[] = R"({
  "profile_id": "p1",
  "area_osm_id": 10,
  "unclaimed": false,
  "contested": false,
  "stale": false,
  "participant_count": 4,
  "boss": {"nickname": null, "decayed_score": 3.2, "is_current_user": false},
  "ranking": [
    {
      "rank": 1,
      "nickname": null,
      "decayed_score": 3.2,
      "gap_to_leader": 0.0,
      "gap_to_current_user": 1.1,
      "is_current_user": false,
      "eligible": true
    }
  ]
})";

char const kNamedNicknameJson[] = R"({
  "profile_id": "p1",
  "area_osm_id": 10,
  "unclaimed": false,
  "contested": false,
  "stale": true,
  "participant_count": 4,
  "boss": {"nickname": "Ada", "decayed_score": 4.0, "is_current_user": true},
  "ranking": [
    {
      "rank": 1,
      "nickname": "Ada",
      "decayed_score": 4.0,
      "gap_to_leader": 0.0,
      "gap_to_current_user": null,
      "is_current_user": true,
      "eligible": true
    }
  ]
})";

class SnapshotCleanup
{
public:
  SnapshotCleanup()
  {
    backend::SetApiBaseUrl("");
    street_pixels::SetCompetitionGetFnForTesting({});
    street_pixels::ClearCompetitionSnapshotCacheForTesting();
    street_pixels::SetCompetitionMapMode(street_pixels::CompetitionMapMode::Explore);
  }

  ~SnapshotCleanup()
  {
    street_pixels::SetCompetitionGetFnForTesting({});
    street_pixels::ClearCompetitionSnapshotCacheForTesting();
    street_pixels::SetCompetitionMapMode(street_pixels::CompetitionMapMode::Explore);
    backend::SetApiBaseUrl("");
  }
};
}  // namespace

UNIT_TEST(CompetitionSnapshot_ParseNullNickname)
{
  SnapshotCleanup cleanup;
  street_pixels::CompetitionAreaSnapshot snap;
  TEST(street_pixels::ParseAreaSnapshotJson(kNullNicknameJson, snap), ());
  TEST(snap.m_boss.has_value(), ());
  TEST(!snap.m_boss->m_nickname.has_value(), ());
  TEST_EQUAL(snap.m_ranking.size(), 1, ());
  TEST(!snap.m_ranking[0].m_nickname.has_value(), ());
}

UNIT_TEST(CompetitionSnapshot_ParseNamedNickname)
{
  SnapshotCleanup cleanup;
  street_pixels::CompetitionAreaSnapshot snap;
  TEST(street_pixels::ParseAreaSnapshotJson(kNamedNicknameJson, snap), ());
  TEST(snap.m_boss.has_value(), ());
  TEST(snap.m_boss->m_nickname.has_value(), ());
  TEST_EQUAL(*snap.m_boss->m_nickname, "Ada", ());
  TEST_EQUAL(snap.m_ranking.size(), 1, ());
  TEST(snap.m_ranking[0].m_nickname.has_value(), ());
  TEST_EQUAL(*snap.m_ranking[0].m_nickname, "Ada", ());
}

UNIT_TEST(CompetitionSnapshot_EmptyApiBaseNoGet)
{
  SnapshotCleanup cleanup;
  backend::SetApiBaseUrl("");
  size_t calls = 0;
  street_pixels::SetCompetitionGetFnForTesting(
      [&calls](std::string const &, std::string &,
               std::vector<std::pair<std::string, std::string>> const &)
      {
        ++calls;
        return 200;
      });
  auto const result = street_pixels::FetchAreaSnapshot(10, "pid-1");
  TEST_EQUAL(calls, 0, ());
  TEST(!result.m_didGet, ());
  TEST(result.m_chrome.m_offline, ());
}

UNIT_TEST(CompetitionSnapshot_HttpFailUsesCacheAndStaleOrOffline)
{
  SnapshotCleanup cleanup;
  backend::SetApiBaseUrl("https://example.com/api");
  int status = 200;
  std::string body = kNamedNicknameJson;
  street_pixels::SetCompetitionGetFnForTesting(
      [&status, &body](std::string const &, std::string & response,
                       std::vector<std::pair<std::string, std::string>> const &)
      {
        response = body;
        return status;
      });
  auto const ok = street_pixels::FetchAreaSnapshot(10, "pid-1");
  TEST(ok.m_snapshot.has_value(), ());
  TEST_EQUAL(*ok.m_snapshot->m_boss->m_nickname, "Ada", ());
  TEST(!ok.m_chrome.m_offline, ());

  status = 500;
  body.clear();
  auto const fail = street_pixels::FetchAreaSnapshot(10, "pid-1");
  TEST(fail.m_chrome.m_offline || fail.m_chrome.m_stale, ());
  TEST(fail.m_snapshot.has_value(), ());
  TEST_EQUAL(*fail.m_snapshot->m_boss->m_nickname, "Ada", ());
}

UNIT_TEST(CompetitionSnapshot_UrlHasProfileQueryNoFriendsHeaders)
{
  SnapshotCleanup cleanup;
  backend::SetApiBaseUrl("https://example.com/api");
  std::string seenUrl;
  std::vector<std::pair<std::string, std::string>> seenHeaders;
  street_pixels::SetCompetitionGetFnForTesting(
      [&seenUrl, &seenHeaders](std::string const & url, std::string & response,
                               std::vector<std::pair<std::string, std::string>> const & headers)
      {
        seenUrl = url;
        seenHeaders = headers;
        response = kNamedNicknameJson;
        return 200;
      });
  auto const result = street_pixels::FetchAreaSnapshot(10, "pid-1");
  TEST(result.m_didGet, ());
  TEST(seenUrl.find("profile_id=") != std::string::npos, (seenUrl));
  TEST(seenUrl.find("/stats/upload") == std::string::npos, (seenUrl));
  TEST(seenUrl.find("X-Device-Id") == std::string::npos, (seenUrl));
  TEST(seenHeaders.empty(), ());
  for (auto const & header : seenHeaders)
  {
    TEST(header.first != "X-Device-Id", ());
    TEST(header.first != "X-Username", ());
  }
}
