#include "map/product_analytics.hpp"

#include "street_pixels_areas/weekly_city_week.hpp"

#include "platform/settings.hpp"

#include "base/timer.hpp"

#include <sstream>

namespace street_pixels
{
namespace
{
void IncrementProductCounter(std::string_view key)
{
  uint64_t value = 0;
  settings::TryGet(key, value);
  settings::Set(key, value + 1);
}

void IncrementProductOnce(std::string_view key)
{
  uint64_t value = 0;
  settings::TryGet(key, value);
  if (value != 0)
    return;
  settings::Set(key, static_cast<uint64_t>(1));
}
}  // namespace

void ProductAnalytics::RecordPositionPermissionGranted()
{
  IncrementProductOnce(kPositionPermissionGrantedKey);
}

void ProductAnalytics::RecordNotifyPermissionGranted()
{
  IncrementProductOnce(kNotifyPermissionGrantedKey);
}

void ProductAnalytics::RecordRecordingStarted()
{
  IncrementProductOnce(kFirstRecordingStartedKey);
  IncrementProductCounter(kRecordingSessionsKey);
}

void ProductAnalytics::RecordRecordingCompleted()
{
  IncrementProductOnce(kFirstRecordingCompletedKey);
}

void ProductAnalytics::RecordLiveCollected(uint64_t count)
{
  RecordLiveCollectedAt(count, static_cast<int64_t>(base::SecondsSinceEpoch()));
}

void ProductAnalytics::RecordLiveCollectedAt(uint64_t count, int64_t nowUnix)
{
  if (count == 0)
    return;

  auto const bounds = WeekBoundsAtFixedOffset(nowUnix, 0);
  uint64_t const weekId = bounds.m_weekId < 0 ? 0 : static_cast<uint64_t>(bounds.m_weekId);
  uint64_t storedWeek = 0;
  settings::TryGet(kNewCollectedWeekIdKey, storedWeek);
  uint64_t current = 0;
  if (storedWeek == weekId)
    settings::TryGet(kNewCollectedThisWeekKey, current);
  settings::Set(kNewCollectedWeekIdKey, weekId);
  settings::Set(kNewCollectedThisWeekKey, current + count);

  uint64_t total = 0;
  settings::TryGet(kLiveCollectedTotalKey, total);
  uint64_t const next = total + count;
  settings::Set(kLiveCollectedTotalKey, next);
  if (total == 0)
    IncrementProductOnce(kFirstCollectedKey);
  if (total < 10 && next >= 10)
    IncrementProductOnce(kFirstTenCollectedKey);
}

void ProductAnalytics::RecordFirstGoalComplete()
{
  IncrementProductOnce(kFirstGoalCompleteKey);
}

void ProductAnalytics::RecordPlacesWithProgress()
{
  IncrementProductCounter(kPlacesWithProgressKey);
}

void ProductAnalytics::RecordFirstMilestone25()
{
  IncrementProductOnce(kFirstMilestone25Key);
}

void ProductAnalytics::RecordFirstMilestone50()
{
  IncrementProductOnce(kFirstMilestone50Key);
}

void ProductAnalytics::RecordFirstComplete()
{
  IncrementProductOnce(kFirstCompleteKey);
}

void ProductAnalytics::RecordCompetitionPromptViewed()
{
  IncrementProductCounter(kCompetitionPromptViewedKey);
}

void ProductAnalytics::RecordCompetitionOptIn()
{
  IncrementProductCounter(kCompetitionOptInKey);
}

void ProductAnalytics::RecordLeadershipQualified()
{
  IncrementProductOnce(kLeadershipQualifiedKey);
}

void ProductAnalytics::RecordBecameBoss()
{
  IncrementProductOnce(kBecameBossKey);
}

void ProductAnalytics::RecordBecameContested()
{
  IncrementProductOnce(kBecameContestedKey);
}

void ProductAnalytics::RecordBecameUnclaimed()
{
  IncrementProductOnce(kBecameUnclaimedKey);
}

void ProductAnalytics::RecordWeeklyBoardUsed()
{
  IncrementProductCounter(kWeeklyBoardUsedKey);
}

ProductAnalyticsSnapshot ProductAnalytics::LoadSnapshot()
{
  ProductAnalyticsSnapshot snapshot;
  settings::TryGet(kPositionPermissionGrantedKey, snapshot.m_positionPermissionGranted);
  settings::TryGet(kNotifyPermissionGrantedKey, snapshot.m_notifyPermissionGranted);
  settings::TryGet(kFirstRecordingStartedKey, snapshot.m_firstRecordingStarted);
  settings::TryGet(kFirstCollectedKey, snapshot.m_firstCollected);
  settings::TryGet(kFirstTenCollectedKey, snapshot.m_firstTenCollected);
  settings::TryGet(kFirstGoalCompleteKey, snapshot.m_firstGoalComplete);
  settings::TryGet(kFirstRecordingCompletedKey, snapshot.m_firstRecordingCompleted);
  settings::TryGet(kRecordingSessionsKey, snapshot.m_recordingSessions);
  settings::TryGet(kNewCollectedThisWeekKey, snapshot.m_newCollectedThisWeek);
  settings::TryGet(kPlacesWithProgressKey, snapshot.m_placesWithProgress);
  settings::TryGet(kFirstMilestone25Key, snapshot.m_firstMilestone25);
  settings::TryGet(kFirstMilestone50Key, snapshot.m_firstMilestone50);
  settings::TryGet(kFirstCompleteKey, snapshot.m_firstComplete);
  settings::TryGet(kCompetitionPromptViewedKey, snapshot.m_competitionPromptViewed);
  settings::TryGet(kCompetitionOptInKey, snapshot.m_competitionOptIn);
  settings::TryGet(kLeadershipQualifiedKey, snapshot.m_leadershipQualified);
  settings::TryGet(kBecameBossKey, snapshot.m_becameBoss);
  settings::TryGet(kBecameContestedKey, snapshot.m_becameContested);
  settings::TryGet(kBecameUnclaimedKey, snapshot.m_becameUnclaimed);
  settings::TryGet(kWeeklyBoardUsedKey, snapshot.m_weeklyBoardUsed);
  return snapshot;
}

std::array<std::pair<std::string_view, uint64_t>, kProductAnalyticsCounterCount> ProductAnalytics::SerializedSnapshot()
{
  ProductAnalyticsSnapshot const snapshot = LoadSnapshot();
  return {{
      {kPositionPermissionGrantedName, snapshot.m_positionPermissionGranted},
      {kNotifyPermissionGrantedName, snapshot.m_notifyPermissionGranted},
      {kFirstRecordingStartedName, snapshot.m_firstRecordingStarted},
      {kFirstCollectedName, snapshot.m_firstCollected},
      {kFirstTenCollectedName, snapshot.m_firstTenCollected},
      {kFirstGoalCompleteName, snapshot.m_firstGoalComplete},
      {kFirstRecordingCompletedName, snapshot.m_firstRecordingCompleted},
      {kRecordingSessionsName, snapshot.m_recordingSessions},
      {kNewCollectedThisWeekName, snapshot.m_newCollectedThisWeek},
      {kPlacesWithProgressName, snapshot.m_placesWithProgress},
      {kFirstMilestone25Name, snapshot.m_firstMilestone25},
      {kFirstMilestone50Name, snapshot.m_firstMilestone50},
      {kFirstCompleteName, snapshot.m_firstComplete},
      {kCompetitionPromptViewedName, snapshot.m_competitionPromptViewed},
      {kCompetitionOptInName, snapshot.m_competitionOptIn},
      {kLeadershipQualifiedName, snapshot.m_leadershipQualified},
      {kBecameBossName, snapshot.m_becameBoss},
      {kBecameContestedName, snapshot.m_becameContested},
      {kBecameUnclaimedName, snapshot.m_becameUnclaimed},
      {kWeeklyBoardUsedName, snapshot.m_weeklyBoardUsed},
  }};
}

void ProductAnalytics::ResetForTesting()
{
  settings::Delete(kPositionPermissionGrantedKey);
  settings::Delete(kNotifyPermissionGrantedKey);
  settings::Delete(kFirstRecordingStartedKey);
  settings::Delete(kFirstCollectedKey);
  settings::Delete(kFirstTenCollectedKey);
  settings::Delete(kFirstGoalCompleteKey);
  settings::Delete(kFirstRecordingCompletedKey);
  settings::Delete(kRecordingSessionsKey);
  settings::Delete(kNewCollectedThisWeekKey);
  settings::Delete(kPlacesWithProgressKey);
  settings::Delete(kFirstMilestone25Key);
  settings::Delete(kFirstMilestone50Key);
  settings::Delete(kFirstCompleteKey);
  settings::Delete(kCompetitionPromptViewedKey);
  settings::Delete(kCompetitionOptInKey);
  settings::Delete(kLeadershipQualifiedKey);
  settings::Delete(kBecameBossKey);
  settings::Delete(kBecameContestedKey);
  settings::Delete(kBecameUnclaimedKey);
  settings::Delete(kWeeklyBoardUsedKey);
  settings::Delete(kNewCollectedWeekIdKey);
  settings::Delete(kLiveCollectedTotalKey);
}

std::string DebugPrint(ProductAnalyticsSnapshot const & snapshot)
{
  std::ostringstream oss;
  oss << "position-permission-granted=" << snapshot.m_positionPermissionGranted
      << " notify-permission-granted=" << snapshot.m_notifyPermissionGranted
      << " first-recording-started=" << snapshot.m_firstRecordingStarted
      << " first-collected=" << snapshot.m_firstCollected << " first-ten-collected=" << snapshot.m_firstTenCollected
      << " first-goal-complete=" << snapshot.m_firstGoalComplete
      << " first-recording-completed=" << snapshot.m_firstRecordingCompleted
      << " recording-sessions=" << snapshot.m_recordingSessions
      << " new-collected-this-week=" << snapshot.m_newCollectedThisWeek
      << " places-with-progress=" << snapshot.m_placesWithProgress
      << " first-milestone-25=" << snapshot.m_firstMilestone25 << " first-milestone-50=" << snapshot.m_firstMilestone50
      << " first-complete=" << snapshot.m_firstComplete
      << " competition-prompt-viewed=" << snapshot.m_competitionPromptViewed
      << " competition-opt-in=" << snapshot.m_competitionOptIn
      << " leadership-qualified=" << snapshot.m_leadershipQualified << " became-boss=" << snapshot.m_becameBoss
      << " became-contested=" << snapshot.m_becameContested << " became-unclaimed=" << snapshot.m_becameUnclaimed
      << " weekly-board-used=" << snapshot.m_weeklyBoardUsed;
  return oss.str();
}
}  // namespace street_pixels
