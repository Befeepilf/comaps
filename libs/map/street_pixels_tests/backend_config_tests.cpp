#include "testing/testing.hpp"

#include "map/backend_config.hpp"
#include "map/friends_manager.hpp"
#include "map/identity_store.hpp"

namespace
{
void ClearApiBaseUrl() { backend::SetApiBaseUrl(""); }
}  // namespace

UNIT_TEST(BackendConfig_DefaultEmptyWhenUnset)
{
  ClearApiBaseUrl();
  TEST(backend::GetApiBaseUrl().empty(), ());
  TEST(!backend::IsApiConfigured(), ());
}

UNIT_TEST(BackendConfig_SetNormalizesTrailingSlash)
{
  ClearApiBaseUrl();
  backend::SetApiBaseUrl("https://example.com/api/");
  TEST_EQUAL(backend::GetApiBaseUrl(), "https://example.com/api", ());
  TEST(backend::IsApiConfigured(), ());
  ClearApiBaseUrl();
}

UNIT_TEST(BackendConfig_ClearOnEmptySet)
{
  backend::SetApiBaseUrl("https://example.com/api");
  backend::SetApiBaseUrl("");
  TEST(backend::GetApiBaseUrl().empty(), ());
  TEST(!backend::IsApiConfigured(), ());
}

UNIT_TEST(BackendConfig_StatsUploadUrlEmptyWhenUnconfigured)
{
  ClearApiBaseUrl();
  TEST(backend::GetStatsUploadUrl().empty(), ());
}

UNIT_TEST(BackendConfig_StatsUploadUrlWhenConfigured)
{
  ClearApiBaseUrl();
  backend::SetApiBaseUrl("https://example.com/api");
  TEST_EQUAL(backend::GetStatsUploadUrl(), "https://example.com/api/stats/upload", ());
  ClearApiBaseUrl();
}

UNIT_TEST(BackendConfig_CompetitionAggregatesUrlEmptyWhenUnconfigured)
{
  ClearApiBaseUrl();
  TEST(backend::GetCompetitionAggregatesUrl().empty(), ());
}

UNIT_TEST(BackendConfig_CompetitionAggregatesUrlWhenConfigured)
{
  ClearApiBaseUrl();
  backend::SetApiBaseUrl("https://example.com/api/");
  TEST_EQUAL(backend::GetCompetitionAggregatesUrl(), "https://example.com/api/v1/competition/aggregates", ());
  TEST(backend::GetCompetitionAggregatesUrl() != backend::GetStatsUploadUrl(), ());
  TEST(backend::GetCompetitionAggregatesUrl().find("/stats/upload") == std::string::npos, ());
  ClearApiBaseUrl();
}

UNIT_TEST(BackendConfig_CompetitionAggregatesUrlNeverUsesStatsUpload)
{
  ClearApiBaseUrl();
  backend::SetApiBaseUrl("https://example.com/api");
  std::string const competition = backend::GetCompetitionAggregatesUrl();
  std::string const stats = backend::GetStatsUploadUrl();
  TEST_EQUAL(competition, "https://example.com/api/v1/competition/aggregates", ());
  TEST_EQUAL(stats, "https://example.com/api/stats/upload", ());
  TEST(competition.find(stats) == std::string::npos, ());
  TEST(competition.find("/stats/upload") == std::string::npos, ());
  ClearApiBaseUrl();
}

UNIT_TEST(BackendConfig_CompetitionRegisterUrlEmptyWhenUnconfigured)
{
  ClearApiBaseUrl();
  TEST(backend::GetCompetitionRegisterUrl().empty(), ());
}

UNIT_TEST(BackendConfig_CompetitionRegisterUrlWhenConfigured)
{
  ClearApiBaseUrl();
  backend::SetApiBaseUrl("https://example.com/api/");
  TEST_EQUAL(backend::GetCompetitionRegisterUrl(), "https://example.com/api/v1/competition/register", ());
  TEST(backend::GetCompetitionRegisterUrl().find("/stats/upload") == std::string::npos, ());
  ClearApiBaseUrl();
}

UNIT_TEST(BackendConfig_CompetitionNicknameUrlEmptyWhenUnconfigured)
{
  ClearApiBaseUrl();
  TEST(backend::GetCompetitionNicknameUrl().empty(), ());
}

UNIT_TEST(BackendConfig_CompetitionNicknameUrlWhenConfigured)
{
  ClearApiBaseUrl();
  backend::SetApiBaseUrl("https://example.com/api");
  TEST_EQUAL(backend::GetCompetitionNicknameUrl(), "https://example.com/api/v1/competition/nickname", ());
  TEST(backend::GetCompetitionNicknameUrl().find("/stats/upload") == std::string::npos, ());
  ClearApiBaseUrl();
}

UNIT_TEST(BackendConfig_CompetitionAreaSnapshotUrlEmptyWhenUnconfigured)
{
  ClearApiBaseUrl();
  TEST(backend::GetCompetitionAreaSnapshotUrl(10).empty(), ());
}

UNIT_TEST(BackendConfig_CompetitionAreaSnapshotUrlWhenConfigured)
{
  ClearApiBaseUrl();
  backend::SetApiBaseUrl("https://example.com/api/");
  TEST_EQUAL(backend::GetCompetitionAreaSnapshotUrl(10), "https://example.com/api/v1/competition/areas/10", ());
  TEST_EQUAL(backend::GetCompetitionAreaSnapshotUrl(-123), "https://example.com/api/v1/competition/areas/-123", ());
  TEST(backend::GetCompetitionAreaSnapshotUrl(10).find("/stats/upload") == std::string::npos, ());
  ClearApiBaseUrl();
}

UNIT_TEST(BackendConfig_CompetitionWeeklyBoardUrlEmptyWhenUnconfigured)
{
  ClearApiBaseUrl();
  TEST(backend::GetCompetitionWeeklyBoardUrl(20).empty(), ());
}

UNIT_TEST(BackendConfig_CompetitionWeeklyBoardUrlWhenConfigured)
{
  ClearApiBaseUrl();
  backend::SetApiBaseUrl("https://example.com/api");
  TEST_EQUAL(backend::GetCompetitionWeeklyBoardUrl(20), "https://example.com/api/v1/competition/weekly/20", ());
  TEST(backend::GetCompetitionWeeklyBoardUrl(20).find("/stats/upload") == std::string::npos, ());
  ClearApiBaseUrl();
}

UNIT_TEST(BackendConfig_CompetitionDeleteUrlEmptyWhenUnconfigured)
{
  ClearApiBaseUrl();
  TEST(backend::GetCompetitionDeleteUrl().empty(), ());
}

UNIT_TEST(BackendConfig_CompetitionDeleteUrlWhenConfigured)
{
  ClearApiBaseUrl();
  backend::SetApiBaseUrl("https://example.com/api/");
  TEST_EQUAL(backend::GetCompetitionDeleteUrl(), "https://example.com/api/v1/competition/delete", ());
  TEST(backend::GetCompetitionDeleteUrl().find("/stats/upload") == std::string::npos, ());
  ClearApiBaseUrl();
}

UNIT_TEST(BackendConfig_CompetitionReportUrlEmptyWhenUnconfigured)
{
  ClearApiBaseUrl();
  TEST(backend::GetCompetitionReportUrl().empty(), ());
}

UNIT_TEST(BackendConfig_CompetitionReportUrlWhenConfigured)
{
  ClearApiBaseUrl();
  backend::SetApiBaseUrl("https://example.com/api");
  TEST_EQUAL(backend::GetCompetitionReportUrl(), "https://example.com/api/v1/competition/reports", ());
  TEST(backend::GetCompetitionReportUrl().find("/stats/upload") == std::string::npos, ());
  ClearApiBaseUrl();
}

UNIT_TEST(BackendConfig_CompetitionLeaveUrlEmptyWhenUnconfigured)
{
  ClearApiBaseUrl();
  TEST(backend::GetCompetitionLeaveUrl().empty(), ());
}

UNIT_TEST(BackendConfig_CompetitionLeaveUrlWhenConfigured)
{
  ClearApiBaseUrl();
  backend::SetApiBaseUrl("https://example.com/api/");
  TEST_EQUAL(backend::GetCompetitionLeaveUrl(), "https://example.com/api/v1/competition/leave", ());
  TEST(backend::GetCompetitionLeaveUrl().find("/stats/upload") == std::string::npos, ());
  ClearApiBaseUrl();
}

UNIT_TEST(BackendConfig_CompetitionExportUrlEmptyWhenUnconfigured)
{
  ClearApiBaseUrl();
  TEST(backend::GetCompetitionExportUrl().empty(), ());
}

UNIT_TEST(BackendConfig_CompetitionExportUrlWhenConfigured)
{
  ClearApiBaseUrl();
  backend::SetApiBaseUrl("https://example.com/api");
  TEST_EQUAL(backend::GetCompetitionExportUrl(), "https://example.com/api/v1/competition/export", ());
  TEST(backend::GetCompetitionExportUrl().find("/stats/upload") == std::string::npos, ());
  ClearApiBaseUrl();
}

UNIT_TEST(ExploreStatsUpload_DecisionGateWhenApiUnconfigured)
{
  ClearApiBaseUrl();
  TEST(!backend::IsApiConfigured(), ());
  TEST(backend::GetStatsUploadUrl().empty(), ());
}

UNIT_TEST(FriendsManager_SkipsRefreshWhenApiUnconfigured)
{
  ClearApiBaseUrl();
  FriendsManager manager;
  TEST(!backend::IsApiConfigured(), ());
  manager.Refresh();
  ClearApiBaseUrl();
}

UNIT_TEST(FriendsManager_RefreshNoOpWhenFriendsHiddenInPublicV1)
{
  backend::SetApiBaseUrl("https://example.com/api");
  TEST(!FriendsManager::IsPublicV1Enabled(), ());
  TEST(!FriendsManager::ShouldContactFriendsApi(), ());
  FriendsManager manager;
  manager.Refresh();
  TEST(manager.GetLists().m_accepted.empty(), ());
  TEST(manager.GetLists().m_incoming.empty(), ());
  TEST(manager.GetLists().m_outgoing.empty(), ());
  manager.Signup("Alice_1");
  manager.ChangeUsername("Bob_2");
  manager.SearchByUsername("Alice_1", {});
  manager.SendRequest("u1");
  manager.AcceptRequest("u1");
  manager.CancelRequest("u1");
  manager.DeleteAccount();
  manager.ExportAccount({});
  TEST(!IdentityStore::HasUsername(), ());
  ClearApiBaseUrl();
}
