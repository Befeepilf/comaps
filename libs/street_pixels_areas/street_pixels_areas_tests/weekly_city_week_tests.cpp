#include "testing/testing.hpp"

#include "street_pixels_areas/weekly_city_week.hpp"

#include "base/timegm.hpp"

#include <cstdlib>
#include <ctime>
#include <string>

namespace
{
using namespace street_pixels;

int64_t constexpr kWeeklyCityWeekUtcMonday2026Mar23 = 1774224000;

class ScopedDeviceTimeZone
{
public:
  explicit ScopedDeviceTimeZone(char const * tz)
  {
    char const * prev = std::getenv("TZ");
    if (prev != nullptr)
    {
      m_hadPrev = true;
      m_prev = prev;
    }
    ::setenv("TZ", tz, 1);
    ::tzset();
  }

  ~ScopedDeviceTimeZone()
  {
    if (m_hadPrev)
      ::setenv("TZ", m_prev.c_str(), 1);
    else
      ::unsetenv("TZ");
    ::tzset();
  }

  ScopedDeviceTimeZone(ScopedDeviceTimeZone const &) = delete;
  ScopedDeviceTimeZone & operator=(ScopedDeviceTimeZone const &) = delete;

private:
  bool m_hadPrev = false;
  std::string m_prev;
};
}  // namespace

UNIT_TEST(WeeklyCityWeek_UtcMondayBoundary)
{
  TEST_EQUAL(static_cast<int64_t>(base::TimeGM(2026, 3, 23, 0, 0, 0)), kWeeklyCityWeekUtcMonday2026Mar23, ());

  auto const atStart = WeekBoundsFromUnix(kWeeklyCityWeekUtcMonday2026Mar23, {});
  TEST_EQUAL(atStart.m_weekId, kWeeklyCityWeekUtcMonday2026Mar23, ());
  TEST_EQUAL(atStart.m_weekEndUnix, kWeeklyCityWeekUtcMonday2026Mar23 + kWeeklyCitySecondsPerWeek, ());
  TEST(atStart.m_usedUtcFallback, ());

  int64_t const thursdayUnixWeek =
      (kWeeklyCityWeekUtcMonday2026Mar23 / kWeeklyCitySecondsPerWeek) * kWeeklyCitySecondsPerWeek;
  TEST_NOT_EQUAL(atStart.m_weekId, thursdayUnixWeek, ());

  auto const justBefore = WeekBoundsFromUnix(kWeeklyCityWeekUtcMonday2026Mar23 - 1, {});
  TEST_EQUAL(justBefore.m_weekId, kWeeklyCityWeekUtcMonday2026Mar23 - kWeeklyCitySecondsPerWeek, ());
  TEST_EQUAL(justBefore.m_weekEndUnix, kWeeklyCityWeekUtcMonday2026Mar23, ());
  TEST_NOT_EQUAL(justBefore.m_weekId, atStart.m_weekId, ());

  auto const nextStart = WeekBoundsFromUnix(kWeeklyCityWeekUtcMonday2026Mar23 + kWeeklyCitySecondsPerWeek, {});
  TEST_EQUAL(nextStart.m_weekId, kWeeklyCityWeekUtcMonday2026Mar23 + kWeeklyCitySecondsPerWeek, ());
}

UNIT_TEST(WeeklyCityWeek_FixedOffsetMonday)
{
  int32_t constexpr kUtcPlus2 = 7200;
  int64_t const offsetMonday = kWeeklyCityWeekUtcMonday2026Mar23 - kUtcPlus2;

  auto const atStart = WeekBoundsAtFixedOffset(offsetMonday, kUtcPlus2);
  TEST_EQUAL(atStart.m_weekId, offsetMonday, ());
  TEST_EQUAL(atStart.m_weekEndUnix, offsetMonday + kWeeklyCitySecondsPerWeek, ());
  TEST(!atStart.m_usedUtcFallback, ());

  auto const justBefore = WeekBoundsAtFixedOffset(offsetMonday - 1, kUtcPlus2);
  TEST_EQUAL(justBefore.m_weekId, offsetMonday - kWeeklyCitySecondsPerWeek, ());
  TEST_NOT_EQUAL(justBefore.m_weekId, atStart.m_weekId, ());

  auto const utcAtOffsetMonday = WeekBoundsFromUnix(offsetMonday, {});
  TEST_NOT_EQUAL(utcAtOffsetMonday.m_weekId, atStart.m_weekId, ());
}

UNIT_TEST(WeeklyCityWeek_EmptyTzIsUtc)
{
  auto const emptyTz = WeekBoundsFromUnix(kWeeklyCityWeekUtcMonday2026Mar23, {});
  auto const explicitEmpty = WeekBoundsFromUnix(kWeeklyCityWeekUtcMonday2026Mar23, std::string_view{});
  auto const utcOffset = WeekBoundsAtFixedOffset(kWeeklyCityWeekUtcMonday2026Mar23, 0);
  TEST_EQUAL(emptyTz.m_weekId, utcOffset.m_weekId, ());
  TEST_EQUAL(emptyTz.m_weekEndUnix, utcOffset.m_weekEndUnix, ());
  TEST_EQUAL(explicitEmpty.m_weekId, emptyTz.m_weekId, ());
  TEST(emptyTz.m_usedUtcFallback, ());
}

UNIT_TEST(WeeklyCityWeek_DeviceTzIgnored)
{
  auto const before = WeekBoundsFromUnix(kWeeklyCityWeekUtcMonday2026Mar23 - 1, {});
  ScopedDeviceTimeZone kiritimati("Pacific/Kiritimati");
  auto const after = WeekBoundsFromUnix(kWeeklyCityWeekUtcMonday2026Mar23 - 1, {});
  TEST_EQUAL(after.m_weekId, before.m_weekId, ());
  TEST_EQUAL(after.m_weekEndUnix, before.m_weekEndUnix, ());
  TEST_EQUAL(after.m_weekId, kWeeklyCityWeekUtcMonday2026Mar23 - kWeeklyCitySecondsPerWeek, ());
  TEST(after.m_usedUtcFallback, ());
}

UNIT_TEST(WeeklyCityWeek_RemainingTime)
{
  auto const bounds = WeekBoundsFromUnix(kWeeklyCityWeekUtcMonday2026Mar23, {});
  TEST_EQUAL(RemainingSeconds(bounds, kWeeklyCityWeekUtcMonday2026Mar23), kWeeklyCitySecondsPerWeek, ());
  TEST_EQUAL(RemainingSeconds(bounds, kWeeklyCityWeekUtcMonday2026Mar23 + 1), kWeeklyCitySecondsPerWeek - 1, ());
  TEST_EQUAL(RemainingSeconds(bounds, bounds.m_weekEndUnix - 1), 1, ());
  TEST_EQUAL(RemainingSeconds(bounds, bounds.m_weekEndUnix), 0, ());
  TEST_EQUAL(RemainingSeconds(bounds, bounds.m_weekEndUnix + 10), 0, ());
}
