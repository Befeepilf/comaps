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
