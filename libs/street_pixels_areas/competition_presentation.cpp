#include "street_pixels_areas/competition_presentation.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace street_pixels
{
namespace
{
std::string WithName(char const * prefix, std::string const & name, char const * suffix)
{
  return std::string(prefix) + name + suffix;
}

int CurrentUserRank(std::vector<CompetitionRankingEntry> const & ranking)
{
  for (auto const & row : ranking)
  {
    if (row.m_isCurrentUser)
      return row.m_rank;
  }
  return 0;
}
}  // namespace

std::vector<CompetitionRankingEntry> DedupeRankingRows(std::vector<CompetitionRankingEntry> const & ranking)
{
  std::vector<CompetitionRankingEntry> kept;
  bool seenUser = false;
  for (auto const & row : ranking)
  {
    if (row.m_isCurrentUser)
    {
      if (seenUser)
        continue;
      seenUser = true;
    }
    kept.push_back(row);
  }
  if (kept.size() <= 3)
    return kept;

  bool userInTop3 = false;
  for (size_t i = 0; i < 3; ++i)
  {
    if (kept[i].m_isCurrentUser)
    {
      userInTop3 = true;
      break;
    }
  }
  if (userInTop3)
  {
    kept.resize(3);
    return kept;
  }

  std::vector<CompetitionRankingEntry> out(kept.begin(), kept.begin() + 3);
  for (size_t i = 3; i < kept.size(); ++i)
  {
    if (kept[i].m_isCurrentUser)
    {
      out.push_back(kept[i]);
      break;
    }
  }
  return out;
}

CompetitionCardLineKind ClassifyCompetitionCardLine(CompetitionCardContext const & ctx)
{
  if (!ctx.m_hasConsent || !ctx.m_hasProfile)
    return CompetitionCardLineKind::Empty;
  if (ctx.m_isEligibleBoss)
    return CompetitionCardLineKind::Leading;
  return CompetitionCardLineKind::NotLeading;
}

std::string ComposeCompetitionCardLine(std::string const & areaDisplayName, CompetitionCardContext const & ctx)
{
  auto const kind = ClassifyCompetitionCardLine(ctx);
  if (kind == CompetitionCardLineKind::Empty)
    return {};
  if (kind == CompetitionCardLineKind::Leading)
    return areaDisplayName + " fully explored — and you now lead the area.";
  return areaDisplayName + " fully explored. Revisiting older streets can strengthen your position.";
}

std::string RankingDisplayName(std::optional<std::string> const & nickname, bool isCurrentUser,
                               std::string const & selfNickname)
{
  (void)selfNickname;
  if (isCurrentUser)
    return kYou;
  if (nickname.has_value() && !nickname->empty())
    return *nickname;
  return kAnotherExplorer;
}

std::string ComposeSparseBossLine(std::optional<CompetitionBoss> const & boss, int participantCount)
{
  if (!boss.has_value())
    return {};
  bool const named = participantCount >= 3 && boss->m_nickname.has_value() && !boss->m_nickname->empty();
  if (named)
    return *boss->m_nickname + " currently leads this area";
  if (boss->m_isCurrentUser)
    return kYouLeadThisArea;
  return kAnonymousLeadsThisArea;
}

std::string ComposeSparseGapLine(CompetitionRankingEntry const & otherOrLeader)
{
  double gap = otherOrLeader.m_gapToLeader;
  if (otherOrLeader.m_gapToCurrentUser.has_value())
    gap = *otherOrLeader.m_gapToCurrentUser;
  char buf[96];
  std::snprintf(buf, sizeof(buf), kGapAheadFmt, gap);
  return buf;
}

CompetitionAreaChrome BuildCompetitionAreaChrome(std::optional<CompetitionAreaSnapshot> const & snapshot)
{
  CompetitionAreaChrome chrome;
  if (!snapshot.has_value())
  {
    chrome.m_offline = true;
    return chrome;
  }
  chrome.m_offline = false;
  chrome.m_stale = snapshot->m_stale;
  chrome.m_unclaimed = snapshot->m_unclaimed;
  chrome.m_contested = snapshot->m_contested;
  chrome.m_bossLine = ComposeSparseBossLine(snapshot->m_boss, snapshot->m_participantCount);
  chrome.m_rankingRows = DedupeRankingRows(snapshot->m_ranking);
  bool const userLeads = snapshot->m_boss.has_value() && snapshot->m_boss->m_isCurrentUser;
  if (!userLeads)
  {
    for (auto const & row : chrome.m_rankingRows)
    {
      if (row.m_isCurrentUser)
      {
        chrome.m_gapLine = ComposeSparseGapLine(row);
        break;
      }
    }
    if (chrome.m_gapLine.empty())
    {
      for (auto const & row : chrome.m_rankingRows)
      {
        if (!row.m_isCurrentUser)
        {
          chrome.m_gapLine = ComposeSparseGapLine(row);
          break;
        }
      }
    }
  }
  return chrome;
}

CompetitionHintKind SelectCompetitionHintKind(std::optional<CompetitionAreaSnapshot> const & snapshot,
                                              bool localApproachingEligibility, bool localIsBoss,
                                              bool hasAreaName)
{
  if (!snapshot.has_value())
    return hasAreaName ? CompetitionHintKind::CompareWithArea : CompetitionHintKind::CompareGeneric;
  if (localIsBoss)
    return hasAreaName ? CompetitionHintKind::Leads : CompetitionHintKind::CompareGeneric;
  if (localApproachingEligibility)
    return hasAreaName ? CompetitionHintKind::ApproachingLead : CompetitionHintKind::CompareGeneric;
  bool someoneAhead = false;
  if (snapshot->m_boss.has_value() && !snapshot->m_boss->m_isCurrentUser)
    someoneAhead = true;
  if (!someoneAhead)
  {
    int const userRank = CurrentUserRank(snapshot->m_ranking);
    for (auto const & row : snapshot->m_ranking)
    {
      if (!row.m_isCurrentUser && row.m_rank > 0 && (userRank == 0 || row.m_rank < userRank))
      {
        someoneAhead = true;
        break;
      }
    }
  }
  if (someoneAhead && hasAreaName)
    return CompetitionHintKind::Ahead;
  return hasAreaName ? CompetitionHintKind::CompareWithArea : CompetitionHintKind::CompareGeneric;
}

std::string ComposeCompetitionHintText(CompetitionHintKind kind, std::string const & areaDisplayName)
{
  switch (kind)
  {
  case CompetitionHintKind::Ahead:
    return WithName("Another explorer is ahead of you in ", areaDisplayName, ".");
  case CompetitionHintKind::ApproachingLead:
    return WithName("You're getting close to qualifying for the lead in ", areaDisplayName, ".");
  case CompetitionHintKind::Leads: return WithName("You currently lead ", areaDisplayName, ".");
  case CompetitionHintKind::CompareWithArea:
    return WithName("See how your ", areaDisplayName, " exploration compares.");
  case CompetitionHintKind::CompareGeneric: return kCompetitionHintCompare;
  }
  return kCompetitionHintCompare;
}

std::optional<OvertakingHintKind> SelectOvertakingHintKind(std::optional<CompetitionAreaSnapshot> const & snapshot,
                                                           bool localApproachingEligibility, bool localIsBoss)
{
  if (!snapshot.has_value())
  {
    if (localApproachingEligibility && !localIsBoss)
      return OvertakingHintKind::CloseToQualify;
    return std::nullopt;
  }
  bool const sparse = snapshot->m_participantCount < 3;
  if (localIsBoss && snapshot->m_contested)
    return OvertakingHintKind::LeadNarrowing;
  if (snapshot->m_contested)
    return OvertakingHintKind::AreaContested;
  int const userRank = CurrentUserRank(snapshot->m_ranking);
  if (!localIsBoss && userRank == 2)
    return OvertakingHintKind::CloseToLead;
  if (!sparse && !localIsBoss && userRank == 3)
    return OvertakingHintKind::OneMoreStreetSecond;
  if (localApproachingEligibility && !localIsBoss)
    return OvertakingHintKind::CloseToQualify;
  return std::nullopt;
}

std::string ComposeOvertakingHintText(OvertakingHintKind kind, std::string const & areaDisplayName)
{
  switch (kind)
  {
  case OvertakingHintKind::OneMoreStreetSecond: return kOvertakeSecond;
  case OvertakingHintKind::CloseToLead: return kOvertakeLead;
  case OvertakingHintKind::AreaContested:
    if (areaDisplayName.empty())
      return kOvertakeContested;
    return areaDisplayName + " is contested.";
  case OvertakingHintKind::LeadNarrowing: return kOvertakeNarrowing;
  case OvertakingHintKind::CloseToQualify: return kOvertakeQualify;
  }
  return kOvertakeContested;
}

std::vector<std::string> CompetitionCopyDeniedTokens()
{
  return {"nearby",         "live location", "currently nearby", "last seen",
          "presence",       "coordinates",   "someone is nearby", "invalid"};
}

std::vector<std::string> AllCompetitionCopyEnglish()
{
  return {kCompetitionModeExplore,
          kCompetitionModeCompetition,
          kCompetitionStatusOffline,
          kCompetitionStatusStale,
          kCompetitionUnclaimed,
          kCompetitionContested,
          kCompetitionPersonalCompletionFmt,
          kCompetitionOwnershipScoreFmt,
          kYou,
          kAnotherExplorer,
          kYouLeadThisArea,
          kAnonymousLeadsThisArea,
          kNamedLeadsThisAreaFmt,
          kGapAheadFmt,
          kCompetitionWeeklyTitle,
          kCompetitionWeeklyPixelsFmt,
          kCompetitionWeeklyRemainingFmt,
          kCompetitionWeeklyEmpty,
          kCompetitionCardLeadingFmt,
          kCompetitionCardNotLeadingFmt,
          kCompetitionHintAheadFmt,
          kCompetitionHintApproachingFmt,
          kCompetitionHintLeadsFmt,
          kCompetitionHintCompareAreaFmt,
          kCompetitionHintCompare,
          kCompetitionHintAction,
          kOvertakeSecond,
          kOvertakeLead,
          kOvertakeContestedAreaFmt,
          kOvertakeContested,
          kOvertakeNarrowing,
          kOvertakeQualify,
          kCompetitionLeave,
          kCompetitionDelete,
          kCompetitionLeaveConfirmTitle,
          kCompetitionLeaveConfirmMessage,
          kCompetitionDeleteConfirmTitle,
          kCompetitionDeleteConfirmMessage,
          kCompetitionDeleteUnavailable,
          kCompetitionLeaveDone};
}

std::string DebugPrint(CompetitionMapMode mode)
{
  switch (mode)
  {
  case CompetitionMapMode::Explore: return "Explore";
  case CompetitionMapMode::Competition: return "Competition";
  }
  return "UnknownCompetitionMapMode";
}

std::string DebugPrint(CompetitionHintKind kind)
{
  switch (kind)
  {
  case CompetitionHintKind::Ahead: return "Ahead";
  case CompetitionHintKind::ApproachingLead: return "ApproachingLead";
  case CompetitionHintKind::Leads: return "Leads";
  case CompetitionHintKind::CompareWithArea: return "CompareWithArea";
  case CompetitionHintKind::CompareGeneric: return "CompareGeneric";
  }
  return "UnknownCompetitionHintKind";
}

std::string DebugPrint(OvertakingHintKind kind)
{
  switch (kind)
  {
  case OvertakingHintKind::OneMoreStreetSecond: return "OneMoreStreetSecond";
  case OvertakingHintKind::CloseToLead: return "CloseToLead";
  case OvertakingHintKind::AreaContested: return "AreaContested";
  case OvertakingHintKind::LeadNarrowing: return "LeadNarrowing";
  case OvertakingHintKind::CloseToQualify: return "CloseToQualify";
  }
  return "UnknownOvertakingHintKind";
}

std::string DebugPrint(CompetitionCardLineKind kind)
{
  switch (kind)
  {
  case CompetitionCardLineKind::Empty: return "Empty";
  case CompetitionCardLineKind::Leading: return "Leading";
  case CompetitionCardLineKind::NotLeading: return "NotLeading";
  }
  return "UnknownCompetitionCardLineKind";
}
}  // namespace street_pixels
