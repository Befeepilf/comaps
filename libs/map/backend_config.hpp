#pragma once

#include <string>
#include <string_view>

namespace backend
{
void SetApiBaseUrl(std::string_view url);
std::string GetApiBaseUrl();
bool IsApiConfigured();
std::string GetStatsUploadUrl();
std::string GetCompetitionAggregatesUrl();
std::string GetCompetitionRegisterUrl();
std::string GetCompetitionNicknameUrl();
}
