#include "testing/testing.hpp"

#include "street_pixels_areas/competition_presentation.hpp"

#include "base/string_utils.hpp"

#include <optional>
#include <string>
#include <vector>

namespace
{
bool ContainsIgnoreCase(std::string const & haystack, std::string const & needle)
{
  return strings::MakeLowerCase(haystack).find(strings::MakeLowerCase(needle)) != std::string::npos;
}
}  // namespace

UNIT_TEST(CompetitionCard_LeadingLineWhenEligibleBoss)
{
  street_pixels::CompetitionCardContext ctx;
  ctx.m_hasConsent = true;
  ctx.m_hasProfile = true;
  ctx.m_isEligibleBoss = true;
  TEST_EQUAL(street_pixels::ClassifyCompetitionCardLine(ctx), street_pixels::CompetitionCardLineKind::Leading, ());
  auto const text = street_pixels::ComposeCompetitionCardLine("Kallio", ctx);
  TEST(ContainsIgnoreCase(text, "Kallio"), (text));
  TEST(ContainsIgnoreCase(text, "lead the area"), (text));
  TEST(!ContainsIgnoreCase(text, "invalid"), (text));
  TEST_EQUAL(text, "Kallio fully explored — and you now lead the area.", ());
}

UNIT_TEST(CompetitionCard_NotLeadingWhenEligibleButNotBoss)
{
  street_pixels::CompetitionCardContext ctx;
  ctx.m_hasConsent = true;
  ctx.m_hasProfile = true;
  ctx.m_isEligibleBoss = false;
  TEST_EQUAL(street_pixels::ClassifyCompetitionCardLine(ctx), street_pixels::CompetitionCardLineKind::NotLeading, ());
  auto const text = street_pixels::ComposeCompetitionCardLine("Kallio", ctx);
  TEST(ContainsIgnoreCase(text, "Kallio"), (text));
  TEST(ContainsIgnoreCase(text, "Revisiting older streets"), (text));
  TEST(!ContainsIgnoreCase(text, "invalid"), (text));
}

UNIT_TEST(CompetitionCard_EmptyWithoutConsent)
{
  street_pixels::CompetitionCardContext ctx;
  ctx.m_hasConsent = false;
  ctx.m_hasProfile = true;
  ctx.m_isEligibleBoss = true;
  TEST_EQUAL(street_pixels::ClassifyCompetitionCardLine(ctx), street_pixels::CompetitionCardLineKind::Empty, ());
  TEST(street_pixels::ComposeCompetitionCardLine("Kallio", ctx).empty(), ());
}

UNIT_TEST(CompetitionCard_EmptyWithoutProfile)
{
  street_pixels::CompetitionCardContext ctx;
  ctx.m_hasConsent = true;
  ctx.m_hasProfile = false;
  ctx.m_isEligibleBoss = true;
  TEST_EQUAL(street_pixels::ClassifyCompetitionCardLine(ctx), street_pixels::CompetitionCardLineKind::Empty, ());
  TEST(street_pixels::ComposeCompetitionCardLine("Kallio", ctx).empty(), ());
}

UNIT_TEST(CompetitionRanking_UserInTop3NoDuplicate)
{
  std::vector<street_pixels::CompetitionRankingEntry> ranking(4);
  ranking[0].m_rank = 1;
  ranking[1].m_rank = 2;
  ranking[1].m_isCurrentUser = true;
  ranking[2].m_rank = 3;
  ranking[3].m_rank = 2;
  ranking[3].m_isCurrentUser = true;
  auto const rows = street_pixels::DedupeRankingRows(ranking);
  TEST_EQUAL(rows.size(), 3, ());
  size_t users = 0;
  for (auto const & row : rows)
  {
    if (row.m_isCurrentUser)
      ++users;
  }
  TEST_EQUAL(users, 1, ());
  TEST_EQUAL(rows[1].m_rank, 2, ());
  TEST(rows[1].m_isCurrentUser, ());
}

UNIT_TEST(CompetitionRanking_UserFourthAppendsRow)
{
  std::vector<street_pixels::CompetitionRankingEntry> ranking(4);
  ranking[0].m_rank = 1;
  ranking[1].m_rank = 2;
  ranking[2].m_rank = 3;
  ranking[3].m_rank = 4;
  ranking[3].m_isCurrentUser = true;
  auto const rows = street_pixels::DedupeRankingRows(ranking);
  TEST_EQUAL(rows.size(), 4, ());
  TEST(rows.back().m_isCurrentUser, ());
  TEST_EQUAL(rows.back().m_rank, 4, ());
  TEST(!rows[0].m_isCurrentUser, ());
  TEST(!rows[1].m_isCurrentUser, ());
  TEST(!rows[2].m_isCurrentUser, ());
}

UNIT_TEST(CompetitionRanking_NullNicknamesStayNull)
{
  std::vector<street_pixels::CompetitionRankingEntry> ranking(3);
  ranking[0].m_rank = 1;
  ranking[0].m_nickname = std::nullopt;
  ranking[1].m_rank = 2;
  ranking[1].m_isCurrentUser = true;
  ranking[1].m_nickname = "Ada";
  ranking[2].m_rank = 3;
  ranking[2].m_nickname = std::nullopt;
  auto const rows = street_pixels::DedupeRankingRows(ranking);
  TEST_EQUAL(rows.size(), 3, ());
  TEST(!rows[0].m_nickname.has_value(), ());
  TEST(!rows[2].m_nickname.has_value(), ());
  TEST_EQUAL(street_pixels::RankingDisplayName(rows[0].m_nickname, false, "Ada"),
             street_pixels::kAnotherExplorer, ());
  TEST_EQUAL(street_pixels::RankingDisplayName(rows[1].m_nickname, true, "Ada"), street_pixels::kYou, ());
}

UNIT_TEST(CompetitionSparse_Nlt3AnonymousBossYouLead)
{
  street_pixels::CompetitionBoss boss;
  boss.m_nickname = std::nullopt;
  boss.m_isCurrentUser = true;
  TEST_EQUAL(street_pixels::ComposeSparseBossLine(boss, 2), street_pixels::kYouLeadThisArea, ());
}

UNIT_TEST(CompetitionSparse_Nlt3AnonymousBossOther)
{
  street_pixels::CompetitionBoss boss;
  boss.m_nickname = std::nullopt;
  boss.m_isCurrentUser = false;
  TEST_EQUAL(street_pixels::ComposeSparseBossLine(boss, 2), street_pixels::kAnonymousLeadsThisArea, ());
}

UNIT_TEST(CompetitionSparse_NeverSomeoneIsNearby)
{
  street_pixels::CompetitionBoss you;
  you.m_isCurrentUser = true;
  street_pixels::CompetitionBoss other;
  other.m_isCurrentUser = false;
  std::vector<std::string> sparse = {street_pixels::ComposeSparseBossLine(you, 2),
                                     street_pixels::ComposeSparseBossLine(other, 2),
                                     street_pixels::kAnotherExplorer, street_pixels::kYouLeadThisArea,
                                     street_pixels::kAnonymousLeadsThisArea, street_pixels::kGapAheadFmt};
  street_pixels::CompetitionRankingEntry entry;
  entry.m_gapToLeader = 1.8;
  sparse.push_back(street_pixels::ComposeSparseGapLine(entry));
  for (auto const & text : sparse)
  {
    TEST(!ContainsIgnoreCase(text, "someone is nearby"), (text));
    TEST(!ContainsIgnoreCase(text, "nearby"), (text));
  }
}

UNIT_TEST(CompetitionChrome_StaleFlagFromSnapshot)
{
  street_pixels::CompetitionAreaSnapshot snap;
  snap.m_stale = true;
  snap.m_unclaimed = false;
  snap.m_participantCount = 4;
  auto const chrome = street_pixels::BuildCompetitionAreaChrome(snap);
  TEST(chrome.m_stale, ());
  TEST(!chrome.m_offline, ());
}

UNIT_TEST(CompetitionChrome_OfflineWhenNoSnapshot)
{
  street_pixels::CompetitionAreaChrome chrome = street_pixels::BuildCompetitionAreaChrome(std::nullopt);
  TEST(chrome.m_offline, ());
  chrome.m_localOwnershipScore = 12.5;
  chrome.m_personalCompletionFraction = 0.5;
  TEST_EQUAL(chrome.m_localOwnershipScore, 12.5, ());
  TEST_EQUAL(chrome.m_personalCompletionFraction, 0.5, ());
}

UNIT_TEST(CompetitionCopy_DenyList)
{
  auto const denied = street_pixels::CompetitionCopyDeniedTokens();
  for (auto const & text : street_pixels::AllCompetitionCopyEnglish())
  {
    for (auto const & token : denied)
      TEST(!ContainsIgnoreCase(text, token), (text, token));
  }
}

UNIT_TEST(CompetitionHintCopy_CompareGenericWithoutArea)
{
  auto const kind =
      street_pixels::SelectCompetitionHintKind(std::nullopt, false, false, false);
  TEST_EQUAL(kind, street_pixels::CompetitionHintKind::CompareGeneric, ());
  TEST_EQUAL(street_pixels::ComposeCompetitionHintText(kind, ""),
             "See how your exploration compares.", ());
}

UNIT_TEST(CompetitionHintCopy_CompareWithAreaWithoutSnapshot)
{
  auto const kind =
      street_pixels::SelectCompetitionHintKind(std::nullopt, false, false, true);
  TEST_EQUAL(kind, street_pixels::CompetitionHintKind::CompareWithArea, ());
  TEST_EQUAL(street_pixels::ComposeCompetitionHintText(kind, "Kallio"),
             "See how your Kallio exploration compares.", ());
}
