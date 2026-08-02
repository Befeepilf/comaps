#include "testing/testing.hpp"

#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include <cstdint>

namespace
{
std::int64_t constexpr kNside = 1048576;
std::int64_t constexpr kMaxHealpixId = 12 * kNside * kNside - 1;
std::int64_t constexpr kExploredBit = static_cast<std::int64_t>(0x8000000000000000ULL);
std::int64_t constexpr kEverLiveBit = static_cast<std::int64_t>(0x4000000000000000ULL);
std::int64_t constexpr kPixelIdMask = 0x3FFFFFFFFFFFFFFF;
}  // namespace

UNIT_TEST(StreetPixel_SetExploredPreservesIdentifier)
{
  auto pixel = street_pixels_tests::MakeStreetPixel(42, false);
  TEST_EQUAL(pixel.GetPixelId(), 42, ());
  TEST(!pixel.IsExplored(), ());
  TEST(!pixel.IsEverLive(), ());

  pixel.SetExplored(true);
  TEST(pixel.IsExplored(), ());
  TEST(!pixel.IsEverLive(), ());
  TEST_EQUAL(pixel.GetPixelId(), 42, ());
}

UNIT_TEST(StreetPixel_IdentifierMaskExcludesFlagBits)
{
  auto pixel = street_pixels_tests::MakeStreetPixel(12345, true, true);
  TEST(pixel.IsExplored(), ());
  TEST(pixel.IsEverLive(), ());
  TEST_EQUAL(pixel.GetPixelId(), 12345, ());
  TEST_EQUAL(pixel.GetPixelId() & kExploredBit, 0, ());
  TEST_EQUAL(pixel.GetPixelId() & kEverLiveBit, 0, ());
  TEST_EQUAL(pixel.GetPixelId() & ~kPixelIdMask, 0, ());
}

UNIT_TEST(StreetPixel_MaximalHealpixIdRoundTrip)
{
  auto unexplored = street_pixels_tests::MakeStreetPixel(kMaxHealpixId, false);
  TEST_EQUAL(unexplored.GetPixelId(), kMaxHealpixId, ());
  TEST(!unexplored.IsExplored(), ());
  TEST(!unexplored.IsEverLive(), ());

  auto explored = street_pixels_tests::MakeStreetPixel(kMaxHealpixId, true);
  TEST_EQUAL(explored.GetPixelId(), kMaxHealpixId, ());
  TEST(explored.IsExplored(), ());
  TEST(!explored.IsEverLive(), ());

  explored.SetExplored(true);
  TEST_EQUAL(explored.GetPixelId(), kMaxHealpixId, ());
  TEST(explored.IsExplored(), ());

  explored.SetEverLive(true);
  TEST_EQUAL(explored.GetPixelId(), kMaxHealpixId, ());
  TEST(explored.IsEverLive(), ());
}

UNIT_TEST(StreetPixel_UnexploredByDefault)
{
  auto pixel = street_pixels_tests::MakeStreetPixel(1, false);
  TEST(!pixel.IsExplored(), ());
  TEST(!pixel.IsEverLive(), ());
  TEST_EQUAL(pixel.GetPixelId(), 1, ());
}

UNIT_TEST(StreetPixel_SetExploredFalseIsNoOp)
{
  auto pixel = street_pixels_tests::MakeStreetPixel(7, true, true);
  pixel.SetExplored(false);
  TEST(pixel.IsExplored(), ());
  TEST(pixel.IsEverLive(), ());
  TEST_EQUAL(pixel.GetPixelId(), 7, ());
}

UNIT_TEST(StreetPixel_SetEverLiveFalseIsNoOp)
{
  auto pixel = street_pixels_tests::MakeStreetPixel(7, true, true);
  pixel.SetEverLive(false);
  TEST(pixel.IsExplored(), ());
  TEST(pixel.IsEverLive(), ());
  TEST_EQUAL(pixel.GetPixelId(), 7, ());
}

UNIT_TEST(StreetPixel_UnexploredKeepsEverLiveClear)
{
  auto pixel = street_pixels_tests::MakeStreetPixel(9, false);
  TEST(!pixel.IsExplored(), ());
  TEST(!pixel.IsEverLive(), ());
  TEST_EQUAL(pixel.GetPixelId(), 9, ());
}
