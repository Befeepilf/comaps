#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace street_pixels
{
enum class CompetitionMapMode : uint8_t
{
  Explore = 0,
  Competition = 1
};

enum class CompetitionHintKind : uint8_t
{
  Ahead = 0,
  ApproachingLead = 1,
  Leads = 2,
  CompareWithArea = 3,
  CompareGeneric = 4,
};

enum class OvertakingHintKind : uint8_t
{
  OneMoreStreetSecond = 0,
  CloseToLead = 1,
  AreaContested = 2,
  LeadNarrowing = 3,
  CloseToQualify = 4,
};

enum class CompetitionCardLineKind : uint8_t
{
  Empty = 0,
  Leading = 1,
  NotLeading = 2,
};

struct CompetitionRankingEntry
{
  int m_rank = 0;
  std::optional<std::string> m_nickname;
  double m_decayedScore = 0.0;
  double m_gapToLeader = 0.0;
  std::optional<double> m_gapToCurrentUser;
  bool m_isCurrentUser = false;
  bool m_eligible = false;
};

struct CompetitionBoss
{
  std::optional<std::string> m_nickname;
  double m_decayedScore = 0.0;
  bool m_isCurrentUser = false;
};

struct CompetitionAreaSnapshot
{
  std::string m_profileId;
  int64_t m_areaOsmId = 0;
  bool m_unclaimed = true;
  bool m_contested = false;
  bool m_stale = false;
  int m_participantCount = 0;
  std::optional<CompetitionBoss> m_boss;
  std::vector<CompetitionRankingEntry> m_ranking;
};

struct CompetitionWeeklyEntry
{
  int m_rank = 0;
  std::optional<std::string> m_nickname;
  int64_t m_newLiveCount = 0;
  double m_gapToLeader = 0.0;
  std::optional<double> m_gapToCurrentUser;
  bool m_isCurrentUser = false;
};

struct CompetitionWeeklyBoard
{
  std::string m_profileId;
  int64_t m_cityOsmId = 0;
  int64_t m_weekStartUnix = 0;
  int64_t m_secondsRemaining = 0;
  int m_participantCount = 0;
  std::vector<CompetitionWeeklyEntry> m_ranking;
};

struct CompetitionAreaChrome
{
  bool m_offline = false;
  bool m_stale = false;
  bool m_unclaimed = false;
  bool m_contested = false;
  std::string m_bossLine;
  std::string m_gapLine;
  std::vector<CompetitionRankingEntry> m_rankingRows;
  double m_localOwnershipScore = 0.0;
  double m_personalCompletionFraction = 0.0;
  bool m_localEligible = false;
  bool m_localIsBoss = false;
};

struct CompetitionCardContext
{
  bool m_hasConsent = false;
  bool m_hasProfile = false;
  bool m_isEligibleBoss = false;
};

uint32_t constexpr kCompetitionHintLivePixelThreshold = 30;
uint64_t constexpr kOvertakingHintMinIntervalSeconds = 6 * 60 * 60;

inline constexpr char const kCompetitionModeExplore[] = "Explore";
inline constexpr char const kCompetitionModeCompetition[] = "Competition";
inline constexpr char const kCompetitionStatusOffline[] = "Offline — showing last rankings";
inline constexpr char const kCompetitionStatusStale[] = "Rankings may be out of date";
inline constexpr char const kCompetitionUnclaimed[] = "Unclaimed";
inline constexpr char const kCompetitionContested[] = "Contested";
inline constexpr char const kCompetitionPersonalCompletionFmt[] = "Personal completion: %s";
inline constexpr char const kCompetitionOwnershipScoreFmt[] = "Your ownership score: %.1f";
inline constexpr char const kYou[] = "You";
inline constexpr char const kAnotherExplorer[] = "Another explorer";
inline constexpr char const kYouLeadThisArea[] = "You currently lead this area";
inline constexpr char const kAnonymousLeadsThisArea[] = "Another explorer currently leads this area";
inline constexpr char const kNamedLeadsThisAreaFmt[] = "%s currently leads this area";
inline constexpr char const kGapAheadFmt[] = "Another explorer is %.1f points ahead.";
inline constexpr char const kCompetitionWeeklyTitle[] = "This week";
inline constexpr char const kCompetitionWeeklyPixelsFmt[] = "%d new live pixels this week";
inline constexpr char const kCompetitionWeeklyRemainingFmt[] = "Time remaining this week: %s";
inline constexpr char const kCompetitionWeeklyEmpty[] = "No weekly city ranking yet";

inline constexpr char const kCompetitionCardLeadingFmt[] = "%s fully explored — and you now lead the area.";
inline constexpr char const kCompetitionCardNotLeadingFmt[] =
    "%s fully explored. Revisiting older streets can strengthen your position.";

inline constexpr char const kCompetitionHintAheadFmt[] = "Another explorer is ahead of you in %s.";
inline constexpr char const kCompetitionHintApproachingFmt[] =
    "You're getting close to qualifying for the lead in %s.";
inline constexpr char const kCompetitionHintLeadsFmt[] = "You currently lead %s.";
inline constexpr char const kCompetitionHintCompareAreaFmt[] = "See how your %s exploration compares.";
inline constexpr char const kCompetitionHintCompare[] = "See how your exploration compares.";
inline constexpr char const kCompetitionHintAction[] = "See how";

inline constexpr char const kOvertakeSecond[] = "One more street could move you into second place.";
inline constexpr char const kOvertakeLead[] = "You're close to taking the lead.";
inline constexpr char const kOvertakeContestedAreaFmt[] = "%s is contested.";
inline constexpr char const kOvertakeContested[] = "This area is contested.";
inline constexpr char const kOvertakeNarrowing[] = "Your lead is narrowing.";
inline constexpr char const kOvertakeQualify[] = "You are close to qualifying for leadership.";

inline constexpr char const kCompetitionLeave[] = "Stop competing, keep public stats";
inline constexpr char const kCompetitionDelete[] = "Delete public profile";
inline constexpr char const kCompetitionLeaveConfirmTitle[] = "Stop competing?";
inline constexpr char const kCompetitionLeaveConfirmMessage[] =
    "Uploads will stop. Existing public statistics stay until they expire. Personal exploration on this device is "
    "unchanged.";
inline constexpr char const kCompetitionDeleteConfirmTitle[] = "Delete public profile?";
inline constexpr char const kCompetitionDeleteConfirmMessage[] =
    "Your public nickname and uploaded statistics will be deleted. Personal exploration on this device is unchanged.";
inline constexpr char const kCompetitionDeleteUnavailable[] =
    "Could not delete the public profile. Your local exploration is unchanged.";
inline constexpr char const kCompetitionLeaveDone[] = "Competition is off. Personal exploration is unchanged.";

std::vector<CompetitionRankingEntry> DedupeRankingRows(std::vector<CompetitionRankingEntry> const & ranking);

CompetitionCardLineKind ClassifyCompetitionCardLine(CompetitionCardContext const & ctx);
std::string ComposeCompetitionCardLine(std::string const & areaDisplayName, CompetitionCardContext const & ctx);

std::string RankingDisplayName(std::optional<std::string> const & nickname, bool isCurrentUser,
                               std::string const & selfNickname);
std::string ComposeSparseBossLine(std::optional<CompetitionBoss> const & boss, int participantCount);
std::string ComposeSparseGapLine(CompetitionRankingEntry const & otherOrLeader);

CompetitionAreaChrome BuildCompetitionAreaChrome(std::optional<CompetitionAreaSnapshot> const & snapshot);

CompetitionHintKind SelectCompetitionHintKind(std::optional<CompetitionAreaSnapshot> const & snapshot,
                                              bool localApproachingEligibility, bool localIsBoss,
                                              bool hasAreaName);
std::string ComposeCompetitionHintText(CompetitionHintKind kind, std::string const & areaDisplayName);

std::optional<OvertakingHintKind> SelectOvertakingHintKind(std::optional<CompetitionAreaSnapshot> const & snapshot,
                                                           bool localApproachingEligibility, bool localIsBoss);
std::string ComposeOvertakingHintText(OvertakingHintKind kind, std::string const & areaDisplayName);

std::vector<std::string> CompetitionCopyDeniedTokens();
std::vector<std::string> AllCompetitionCopyEnglish();

std::string DebugPrint(CompetitionMapMode mode);
std::string DebugPrint(CompetitionHintKind kind);
std::string DebugPrint(OvertakingHintKind kind);
std::string DebugPrint(CompetitionCardLineKind kind);
}  // namespace street_pixels
