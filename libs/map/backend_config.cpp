#include "map/backend_config.hpp"

#include "platform/settings.hpp"

#include <string>
#include <string_view>

namespace
{
constexpr char kApiBaseUrlKey[] = "Explore.ApiBaseUrl";

std::string NormalizeBaseUrl(std::string_view url)
{
  std::string out(url);
  while (!out.empty() && out.back() == '/')
    out.pop_back();
  return out;
}
}  // namespace

void backend::SetApiBaseUrl(std::string_view url)
{
  auto const normalized = NormalizeBaseUrl(url);
  if (normalized.empty())
    settings::Delete(std::string_view(kApiBaseUrlKey));
  else
    settings::Set(std::string_view(kApiBaseUrlKey), normalized);
}

std::string backend::GetApiBaseUrl()
{
  std::string value;
  if (settings::Get(std::string_view(kApiBaseUrlKey), value) && !value.empty())
    return NormalizeBaseUrl(value);
  return {};
}

bool backend::IsApiConfigured() { return !GetApiBaseUrl().empty(); }

std::string backend::GetStatsUploadUrl()
{
  auto const base = GetApiBaseUrl();
  if (base.empty())
    return {};
  return base + "/stats/upload";
}

std::string backend::GetCompetitionAggregatesUrl()
{
  auto const base = NormalizeBaseUrl(GetApiBaseUrl());
  if (base.empty())
    return {};
  return base + "/v1/competition/aggregates";
}
