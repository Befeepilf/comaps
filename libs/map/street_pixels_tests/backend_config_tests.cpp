#include "testing/testing.hpp"

#include "map/backend_config.hpp"
#include "map/friends_manager.hpp"

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
