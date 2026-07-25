#include "testing/testing.hpp"

#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include <cstdint>

namespace
{
std::int64_t constexpr kNside = 1048576;
std::int64_t constexpr kMaxHealpixId = 12 * kNside * kNside - 1;
}  // namespace

UNIT_TEST(StreetPixel_SetExploredPreservesIdentifier)
{
  auto pixel = street_pixels_tests::MakeStreetPixel(42, false);
  TEST_EQUAL(pixel.GetPixelId(), 42, ());
  TEST(!pixel.IsExplored(), ());

  pixel.SetExplored(true);
  TEST(pixel.IsExplored(), ());
  TEST_EQUAL(pixel.GetPixelId(), 42, ());
}

UNIT_TEST(StreetPixel_IdentifierMaskExcludesHighBit)
{
  auto pixel = street_pixels_tests::MakeStreetPixel(12345, true);
  TEST(pixel.IsExplored(), ());
  TEST_EQUAL(pixel.GetPixelId(), 12345, ());
  TEST_EQUAL(pixel.GetPixelId() & static_cast<std::int64_t>(0x8000000000000000ULL), 0, ());
}

UNIT_TEST(StreetPixel_MaximalHealpixIdRoundTrip)
{
  auto unexplored = street_pixels_tests::MakeStreetPixel(kMaxHealpixId, false);
  TEST_EQUAL(unexplored.GetPixelId(), kMaxHealpixId, ());
  TEST(!unexplored.IsExplored(), ());

  auto explored = street_pixels_tests::MakeStreetPixel(kMaxHealpixId, true);
  TEST_EQUAL(explored.GetPixelId(), kMaxHealpixId, ());
  TEST(explored.IsExplored(), ());

  explored.SetExplored(true);
  TEST_EQUAL(explored.GetPixelId(), kMaxHealpixId, ());
  TEST(explored.IsExplored(), ());
}

UNIT_TEST(StreetPixel_UnexploredByDefault)
{
  auto pixel = street_pixels_tests::MakeStreetPixel(1, false);
  TEST(!pixel.IsExplored(), ());
  TEST_EQUAL(pixel.GetPixelId(), 1, ());
}
