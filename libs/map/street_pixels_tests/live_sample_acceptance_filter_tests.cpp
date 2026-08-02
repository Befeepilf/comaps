#include "testing/testing.hpp"

#include "map/live_sample_acceptance_filter.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "platform/location.hpp"

namespace
{
double constexpr kBaseLat = 48.2;
double constexpr kBaseLon = 16.37;
double constexpr kBaseTimestamp = 1'000'000.0;
double constexpr kNowSec = kBaseTimestamp + 10.0;

location::GpsInfo MakeSample(double lat, double lon, double accuracyM, double timestampSec,
                             location::TLocationSource source = location::EAndroidNative)
{
  return street_pixels_tests::MakeGpsInfo(lat, lon, accuracyM, timestampSec, source);
}

void AcceptReference(LiveSampleAcceptanceFilter & filter, double lat, double lon, double timestampSec)
{
  auto const result = filter.Evaluate(MakeSample(lat, lon, 5.0, timestampSec), kNowSec);
  TEST(result.accepted, ());
}
}  // namespace

UNIT_TEST(LiveSampleAcceptance_Accuracy24_Accepted)
{
  LiveSampleAcceptanceFilter filter;
  auto const result = filter.Evaluate(MakeSample(kBaseLat, kBaseLon, 24.0, kBaseTimestamp), kNowSec);
  TEST(result.accepted, ());
  TEST_EQUAL(result.reason, SampleRejectReason::None, ());
}

UNIT_TEST(LiveSampleAcceptance_Accuracy26_Rejected)
{
  LiveSampleAcceptanceFilter filter;
  auto const result = filter.Evaluate(MakeSample(kBaseLat, kBaseLon, 26.0, kBaseTimestamp), kNowSec);
  TEST(!result.accepted, ());
  TEST_EQUAL(result.reason, SampleRejectReason::Accuracy, ());
}

UNIT_TEST(LiveSampleAcceptance_MissingAccuracy_Rejected)
{
  LiveSampleAcceptanceFilter filter;
  auto const result = filter.Evaluate(MakeSample(kBaseLat, kBaseLon, 0.0, kBaseTimestamp), kNowSec);
  TEST(!result.accepted, ());
  TEST_EQUAL(result.reason, SampleRejectReason::MissingAccuracy, ());
}

UNIT_TEST(LiveSampleAcceptance_StaleInsideThreshold_Accepted)
{
  LiveSampleAcceptanceFilter filter;
  double const sampleTimestamp = kNowSec - 119.0;
  auto const result = filter.Evaluate(MakeSample(kBaseLat, kBaseLon, 5.0, sampleTimestamp), kNowSec);
  TEST(result.accepted, ());
}

UNIT_TEST(LiveSampleAcceptance_StaleOutsideThreshold_Rejected)
{
  LiveSampleAcceptanceFilter filter;
  double const sampleTimestamp = kNowSec - 121.0;
  auto const result = filter.Evaluate(MakeSample(kBaseLat, kBaseLon, 5.0, sampleTimestamp), kNowSec);
  TEST(!result.accepted, ());
  TEST_EQUAL(result.reason, SampleRejectReason::Stale, ());
}

UNIT_TEST(LiveSampleAcceptance_InvalidSource_Rejected)
{
  LiveSampleAcceptanceFilter filter;
  auto const result =
      filter.Evaluate(MakeSample(kBaseLat, kBaseLon, 5.0, kBaseTimestamp, location::EUndefined), kNowSec);
  TEST(!result.accepted, ());
  TEST_EQUAL(result.reason, SampleRejectReason::Invalid, ());
}

UNIT_TEST(LiveSampleAcceptance_ImpliedSpeed45Kmh_Accepted)
{
  LiveSampleAcceptanceFilter filter;
  AcceptReference(filter, kBaseLat, kBaseLon, kBaseTimestamp);

  double const dtSec = 8.0;
  double const distanceM = 45.0 * 1000.0 / 3600.0 * dtSec;
  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kBaseLat, kBaseLon, distanceM, 0.0);
  auto const result = filter.Evaluate(MakeSample(lat, lon, 5.0, kBaseTimestamp + dtSec), kNowSec);
  TEST(result.accepted, ());
}

UNIT_TEST(LiveSampleAcceptance_ImpliedSpeed55Kmh_Rejected)
{
  LiveSampleAcceptanceFilter filter;
  AcceptReference(filter, kBaseLat, kBaseLon, kBaseTimestamp);

  double const dtSec = 8.0;
  double const distanceM = 55.0 * 1000.0 / 3600.0 * dtSec;
  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kBaseLat, kBaseLon, distanceM, 0.0);
  auto const result = filter.Evaluate(MakeSample(lat, lon, 5.0, kBaseTimestamp + dtSec), kNowSec);
  TEST(!result.accepted, ());
  TEST_EQUAL(result.reason, SampleRejectReason::ImpliedSpeed, ());
}

UNIT_TEST(LiveSampleAcceptance_Teleport_Rejected)
{
  LiveSampleAcceptanceFilter filter;
  AcceptReference(filter, kBaseLat, kBaseLon, kBaseTimestamp);

  double const dtSec = 60.0;
  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kBaseLat, kBaseLon, 250.0, 0.0);
  auto const result = filter.Evaluate(MakeSample(lat, lon, 5.0, kBaseTimestamp + dtSec), kNowSec);
  TEST(!result.accepted, ());
  TEST_EQUAL(result.reason, SampleRejectReason::Teleport, ());
}

UNIT_TEST(LiveSampleAcceptance_RejectionDoesNotMoveReference)
{
  LiveSampleAcceptanceFilter filter;
  AcceptReference(filter, kBaseLat, kBaseLon, kBaseTimestamp);

  double const dtSec = 8.0;
  double const distanceM = 55.0 * 1000.0 / 3600.0 * dtSec;
  auto const [rejectedLat, rejectedLon] = street_pixels_tests::OffsetLatLonByMeters(kBaseLat, kBaseLon, distanceM, 0.0);
  auto const rejected =
      filter.Evaluate(MakeSample(rejectedLat, rejectedLon, 5.0, kBaseTimestamp + dtSec), kNowSec);
  TEST(!rejected.accepted, ());
  TEST_EQUAL(rejected.reason, SampleRejectReason::ImpliedSpeed, ());

  double const walkDistanceM = 5.0 * 1000.0 / 3600.0 * dtSec;
  auto const [acceptedLat, acceptedLon] =
      street_pixels_tests::OffsetLatLonByMeters(kBaseLat, kBaseLon, walkDistanceM, 0.0);
  auto const accepted =
      filter.Evaluate(MakeSample(acceptedLat, acceptedLon, 5.0, kBaseTimestamp + dtSec), kNowSec);
  TEST(accepted.accepted, ());
}

UNIT_TEST(LiveSampleAcceptance_FirstSampleNeverRejectedForSpeed)
{
  LiveSampleAcceptanceFilter filter;
  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kBaseLat, kBaseLon, 500.0, 0.0);
  auto const result = filter.Evaluate(MakeSample(lat, lon, 5.0, kBaseTimestamp), kNowSec);
  TEST(result.accepted, ());
}

UNIT_TEST(LiveSampleAcceptance_ResetClearsReference)
{
  LiveSampleAcceptanceFilter filter;
  AcceptReference(filter, kBaseLat, kBaseLon, kBaseTimestamp);
  TEST(filter.HasAcceptedReference(), ());

  filter.ResetAcceptedReference();
  TEST(!filter.HasAcceptedReference(), ());

  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kBaseLat, kBaseLon, 500.0, 0.0);
  auto const result = filter.Evaluate(MakeSample(lat, lon, 5.0, kBaseTimestamp + 1.0), kNowSec);
  TEST(result.accepted, ());
}

UNIT_TEST(LiveSampleAcceptance_WalkingSequence_FullyAccepted)
{
  LiveSampleAcceptanceFilter filter;
  double const speedMps = 5.0 * 1000.0 / 3600.0;
  double const dtSec = 1.0;
  double const stepM = speedMps * dtSec;

  double lat = kBaseLat;
  double lon = kBaseLon;
  for (size_t i = 0; i < 60; ++i)
  {
    auto const offset = street_pixels_tests::OffsetLatLonByMeters(lat, lon, stepM, 0.0);
    lat = offset.first;
    lon = offset.second;
    auto const result = filter.Evaluate(MakeSample(lat, lon, 5.0, kBaseTimestamp + dtSec * static_cast<double>(i + 1)),
                                        kNowSec);
    TEST(result.accepted, ());
  }
}

UNIT_TEST(LiveSampleAcceptance_CyclingSequence_FullyAccepted)
{
  LiveSampleAcceptanceFilter filter;
  double const speedMps = 25.0 * 1000.0 / 3600.0;
  double const dtSec = 1.0;
  double const stepM = speedMps * dtSec;

  double lat = kBaseLat;
  double lon = kBaseLon;
  for (size_t i = 0; i < 120; ++i)
  {
    auto const offset = street_pixels_tests::OffsetLatLonByMeters(lat, lon, stepM, 0.0);
    lat = offset.first;
    lon = offset.second;
    auto const result = filter.Evaluate(MakeSample(lat, lon, 5.0, kBaseTimestamp + dtSec * static_cast<double>(i + 1)),
                                        kNowSec);
    TEST(result.accepted, ());
  }
}

UNIT_TEST(LiveSampleAcceptance_OneBadFixInSequence_RejectsOnlyThatSample)
{
  LiveSampleAcceptanceFilter filter;
  double const walkSpeedMps = 5.0 * 1000.0 / 3600.0;
  double const dtSec = 1.0;
  double const stepM = walkSpeedMps * dtSec;

  double lat = kBaseLat;
  double lon = kBaseLon;
  size_t rejections = 0;
  for (size_t i = 0; i < 20; ++i)
  {
    auto const offset = street_pixels_tests::OffsetLatLonByMeters(lat, lon, stepM, 0.0);
    lat = offset.first;
    lon = offset.second;
    double const accuracy = (i == 10) ? 40.0 : 5.0;
    auto const result =
        filter.Evaluate(MakeSample(lat, lon, accuracy, kBaseTimestamp + dtSec * static_cast<double>(i + 1)), kNowSec);
    if (!result.accepted)
      ++rejections;
    else
      TEST_EQUAL(result.reason, SampleRejectReason::None, ());
  }
  TEST_EQUAL(rejections, 1, ());
}
