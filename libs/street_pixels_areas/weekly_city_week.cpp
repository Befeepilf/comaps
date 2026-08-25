#include "street_pixels_areas/weekly_city_week.hpp"

#include "base/gmtime.hpp"
#include "base/timegm.hpp"
#include "base/timer.hpp"

#include <ctime>

namespace street_pixels
{
WeeklyCityWeekBounds WeekBoundsAtFixedOffset(int64_t nowUnix, int32_t utcOffsetSec)
{
  int64_t const shifted = nowUnix + static_cast<int64_t>(utcOffsetSec);
  std::tm const tm = base::GmTime(static_cast<time_t>(shifted));
  int const daysFromMonday = (tm.tm_wday + 6) % 7;
  time_t const localMidnight = base::TimeGM(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 0, 0, 0);
  int64_t const mondayLocalAsIfUtc =
      static_cast<int64_t>(localMidnight) - static_cast<int64_t>(daysFromMonday) * kWeeklyCitySecondsPerDay;
  WeeklyCityWeekBounds bounds;
  bounds.m_weekId = mondayLocalAsIfUtc - static_cast<int64_t>(utcOffsetSec);
  bounds.m_weekEndUnix = bounds.m_weekId + kWeeklyCitySecondsPerWeek;
  bounds.m_usedUtcFallback = false;
  return bounds;
}

WeeklyCityWeekBounds WeekBoundsFromUnix(int64_t nowUnix, std::string_view ianaTz)
{
  (void)ianaTz;
  WeeklyCityWeekBounds bounds = WeekBoundsAtFixedOffset(nowUnix, 0);
  bounds.m_usedUtcFallback = true;
  return bounds;
}

int64_t RemainingSeconds(WeeklyCityWeekBounds const & bounds, int64_t nowUnix)
{
  int64_t const remaining = bounds.m_weekEndUnix - nowUnix;
  if (remaining < 0)
    return 0;
  return remaining;
}
}  // namespace street_pixels
