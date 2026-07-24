#pragma once

#include <string>
#include <string_view>

namespace backend
{
void SetApiBaseUrl(std::string_view url);
std::string GetApiBaseUrl();
std::string GetStatsUploadUrl();
}
