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

char const kWeeklyBoardJson[] = R"({
  "profile_id": "p1",
  "city_osm_id": 20,
  "week_start_unix": 1700000000,
  "seconds_remaining": 90000,
  "participant_count": 2,
  "ranking": [
    {
      "rank": 1,
      "nickname": "Ada",
      "new_live_count": 12,
      "gap_to_leader": 0.0,
      "gap_to_current_user": null,
      "is_current_user": true
    },
    {
      "rank": 2,
      "nickname": null,
      "new_live_count": 5,
      "gap_to_leader": 7.0,
      "gap_to_current_user": 7.0,
      "is_current_user": false
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
    street_pixels::ClearCompetitionWeeklyCacheForTesting();
    street_pixels::SetCompetitionMapMode(street_pixels::CompetitionMapMode::Explore);
  }

  ~SnapshotCleanup()
  {
    street_pixels::SetCompetitionGetFnForTesting({});
    street_pixels::ClearCompetitionSnapshotCacheForTesting();
    street_pixels::ClearCompetitionWeeklyCacheForTesting();
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

UNIT_TEST(CompetitionWeekly_ParseAndFetchRoundTrip)
{
  SnapshotCleanup cleanup;
  street_pixels::CompetitionWeeklyBoard parsed;
  TEST(street_pixels::ParseWeeklyBoardJson(kWeeklyBoardJson, parsed), ());
  TEST_EQUAL(parsed.m_profileId, "p1", ());
  TEST_EQUAL(parsed.m_cityOsmId, 20, ());
  TEST_EQUAL(parsed.m_weekStartUnix, 1700000000, ());
  TEST_EQUAL(parsed.m_secondsRemaining, 90000, ());
  TEST_EQUAL(parsed.m_participantCount, 2, ());
  TEST_EQUAL(parsed.m_ranking.size(), 2, ());
  TEST_EQUAL(parsed.m_ranking[0].m_rank, 1, ());
  TEST(parsed.m_ranking[0].m_nickname.has_value(), ());
  TEST_EQUAL(*parsed.m_ranking[0].m_nickname, "Ada", ());
  TEST_EQUAL(parsed.m_ranking[0].m_newLiveCount, 12, ());
  TEST(parsed.m_ranking[0].m_isCurrentUser, ());
  TEST(!parsed.m_ranking[1].m_nickname.has_value(), ());
  TEST_EQUAL(parsed.m_ranking[1].m_newLiveCount, 5, ());
  TEST(!parsed.m_ranking[1].m_isCurrentUser, ());

  auto const chrome = street_pixels::BuildCompetitionWeeklyChrome(parsed);
  TEST(!chrome.m_offline, ());
  TEST(chrome.m_body.find("12 new live pixels this week") != std::string::npos, (chrome.m_body));
  TEST(chrome.m_body.find("1d 1h") != std::string::npos, (chrome.m_body));
  TEST_EQUAL(chrome.m_rows.size(), 2, ());
  TEST(chrome.m_rows[0].find("You") != std::string::npos, (chrome.m_rows[0]));
  TEST(chrome.m_rows[0].find("12") != std::string::npos, (chrome.m_rows[0]));
  TEST(chrome.m_rows[1].find("Another explorer") != std::string::npos, (chrome.m_rows[1]));

  backend::SetApiBaseUrl("https://example.com/api");
  std::string seenUrl;
  street_pixels::SetCompetitionGetFnForTesting(
      [&seenUrl](std::string const & url, std::string & response,
                 std::vector<std::pair<std::string, std::string>> const & headers)
      {
        TEST(headers.empty(), ());
        seenUrl = url;
        response = kWeeklyBoardJson;
        return 200;
      });
  auto const result = street_pixels::FetchWeeklyBoard(20, "pid-1");
  TEST(result.m_didGet, ());
  TEST_EQUAL(result.m_httpStatus, 200, ());
  TEST(seenUrl.find("profile_id=") != std::string::npos, (seenUrl));
  TEST(seenUrl.find("/weekly/") != std::string::npos, (seenUrl));
  TEST(result.m_board.has_value(), ());
  TEST_EQUAL(result.m_board->m_cityOsmId, 20, ());
  TEST_EQUAL(result.m_board->m_ranking.size(), 2, ());
  TEST(result.m_board->m_ranking[0].m_isCurrentUser, ());
  TEST_EQUAL(result.m_board->m_ranking[0].m_newLiveCount, 12, ());
  auto const last = street_pixels::LastWeeklyBoard();
  TEST(last.has_value(), ());
  TEST_EQUAL(last->m_ranking.size(), 2, ());
  TEST_EQUAL(*last->m_ranking[0].m_nickname, "Ada", ());
  TEST(!result.m_chrome.m_offline, ());
  TEST_EQUAL(result.m_chrome.m_rows.size(), 2, ());
  TEST(result.m_chrome.m_body.find("12 new live pixels this week") != std::string::npos, (result.m_chrome.m_body));
}
