#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace street_pixels
{
uint8_t constexpr kProductAnalyticsCounterCount = 20;

struct ProductAnalyticsSnapshot
{
  uint64_t m_positionPermissionGranted = 0;
  uint64_t m_notifyPermissionGranted = 0;
  uint64_t m_firstRecordingStarted = 0;
  uint64_t m_firstCollected = 0;
  uint64_t m_firstTenCollected = 0;
  uint64_t m_firstGoalComplete = 0;
  uint64_t m_firstRecordingCompleted = 0;
  uint64_t m_recordingSessions = 0;
  uint64_t m_newCollectedThisWeek = 0;
  uint64_t m_placesWithProgress = 0;
  uint64_t m_firstMilestone25 = 0;
  uint64_t m_firstMilestone50 = 0;
  uint64_t m_firstComplete = 0;
  uint64_t m_competitionPromptViewed = 0;
  uint64_t m_competitionOptIn = 0;
  uint64_t m_leadershipQualified = 0;
  uint64_t m_becameBoss = 0;
  uint64_t m_becameContested = 0;
  uint64_t m_becameUnclaimed = 0;
  uint64_t m_weeklyBoardUsed = 0;
};

class ProductAnalytics
{
public:
  static std::string_view constexpr kPositionPermissionGrantedKey = "Explore.PositionPermissionGranted";
  static std::string_view constexpr kNotifyPermissionGrantedKey = "Explore.NotifyPermissionGranted";
  static std::string_view constexpr kFirstRecordingStartedKey = "Explore.FirstRecordingStarted";
  static std::string_view constexpr kFirstCollectedKey = "Explore.FirstCollected";
  static std::string_view constexpr kFirstTenCollectedKey = "Explore.FirstTenCollected";
  static std::string_view constexpr kFirstGoalCompleteKey = "Explore.FirstGoalComplete";
  static std::string_view constexpr kFirstRecordingCompletedKey = "Explore.FirstRecordingCompleted";
  static std::string_view constexpr kRecordingSessionsKey = "Explore.RecordingSessions";
  static std::string_view constexpr kNewCollectedThisWeekKey = "Explore.NewCollectedThisWeek";
  static std::string_view constexpr kPlacesWithProgressKey = "Explore.PlacesWithProgress";
  static std::string_view constexpr kFirstMilestone25Key = "Explore.FirstMilestone25";
  static std::string_view constexpr kFirstMilestone50Key = "Explore.FirstMilestone50";
  static std::string_view constexpr kFirstCompleteKey = "Explore.FirstComplete";
  static std::string_view constexpr kCompetitionPromptViewedKey = "Explore.CompetitionPromptViewed";
  static std::string_view constexpr kCompetitionOptInKey = "Explore.CompetitionOptIn";
  static std::string_view constexpr kLeadershipQualifiedKey = "Explore.LeadershipQualified";
  static std::string_view constexpr kBecameBossKey = "Explore.BecameBoss";
  static std::string_view constexpr kBecameContestedKey = "Explore.BecameContested";
  static std::string_view constexpr kBecameUnclaimedKey = "Explore.BecameUnclaimed";
  static std::string_view constexpr kWeeklyBoardUsedKey = "Explore.WeeklyBoardUsed";
  static std::string_view constexpr kNewCollectedWeekIdKey = "Explore.NewCollectedWeekId";
  static std::string_view constexpr kLiveCollectedTotalKey = "Explore.LiveCollectedTotal";

  static std::string_view constexpr kPositionPermissionGrantedName = "position-permission-granted";
  static std::string_view constexpr kNotifyPermissionGrantedName = "notify-permission-granted";
  static std::string_view constexpr kFirstRecordingStartedName = "first-recording-started";
  static std::string_view constexpr kFirstCollectedName = "first-collected";
  static std::string_view constexpr kFirstTenCollectedName = "first-ten-collected";
  static std::string_view constexpr kFirstGoalCompleteName = "first-goal-complete";
  static std::string_view constexpr kFirstRecordingCompletedName = "first-recording-completed";
  static std::string_view constexpr kRecordingSessionsName = "recording-sessions";
  static std::string_view constexpr kNewCollectedThisWeekName = "new-collected-this-week";
  static std::string_view constexpr kPlacesWithProgressName = "places-with-progress";
  static std::string_view constexpr kFirstMilestone25Name = "first-milestone-25";
  static std::string_view constexpr kFirstMilestone50Name = "first-milestone-50";
  static std::string_view constexpr kFirstCompleteName = "first-complete";
  static std::string_view constexpr kCompetitionPromptViewedName = "competition-prompt-viewed";
  static std::string_view constexpr kCompetitionOptInName = "competition-opt-in";
  static std::string_view constexpr kLeadershipQualifiedName = "leadership-qualified";
  static std::string_view constexpr kBecameBossName = "became-boss";
  static std::string_view constexpr kBecameContestedName = "became-contested";
  static std::string_view constexpr kBecameUnclaimedName = "became-unclaimed";
  static std::string_view constexpr kWeeklyBoardUsedName = "weekly-board-used";

  static void RecordPositionPermissionGranted();
  static void RecordNotifyPermissionGranted();
  static void RecordRecordingStarted();
  static void RecordRecordingCompleted();
  static void RecordLiveCollected(uint64_t count);
  static void RecordLiveCollectedAt(uint64_t count, int64_t nowUnix);
  static void RecordFirstGoalComplete();
  static void RecordPlacesWithProgress();
  static void RecordFirstMilestone25();
  static void RecordFirstMilestone50();
  static void RecordFirstComplete();
  static void RecordCompetitionPromptViewed();
  static void RecordCompetitionOptIn();
  static void RecordLeadershipQualified();
  static void RecordBecameBoss();
  static void RecordBecameContested();
  static void RecordBecameUnclaimed();
  static void RecordWeeklyBoardUsed();
  static ProductAnalyticsSnapshot LoadSnapshot();
  static std::array<std::pair<std::string_view, uint64_t>, kProductAnalyticsCounterCount> SerializedSnapshot();
  static void ResetForTesting();
};

std::string DebugPrint(ProductAnalyticsSnapshot const & snapshot);
}  // namespace street_pixels
