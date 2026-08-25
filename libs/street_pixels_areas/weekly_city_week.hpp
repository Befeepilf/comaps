#pragma once

#include <cstdint>
#include <string_view>

namespace street_pixels
{
int64_t constexpr kWeeklyCitySecondsPerDay = 86400;
int64_t constexpr kWeeklyCitySecondsPerWeek = 7 * kWeeklyCitySecondsPerDay;

struct WeeklyCityWeekBounds
{
  int64_t m_weekId = 0;
  int64_t m_weekEndUnix = 0;
  bool m_usedUtcFallback = true;
};

WeeklyCityWeekBounds WeekBoundsFromUnix(int64_t nowUnix, std::string_view ianaTz);
WeeklyCityWeekBounds WeekBoundsAtFixedOffset(int64_t nowUnix, int32_t utcOffsetSec);
int64_t RemainingSeconds(WeeklyCityWeekBounds const & bounds, int64_t nowUnix);
}  // namespace street_pixels
