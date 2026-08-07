#include "testing/testing.hpp"

#include "street_pixels_areas/focus_selection_engine.hpp"

namespace
{
using namespace street_pixels;

UNIT_TEST(FocusSelection_Rule1_RecordingFollowsUserArea)
{
  FocusSelectionRequest req;
  req.m_event = FocusEvent::RecordingOrUserLocation;
  req.m_recordingActive = true;
  req.m_userAreaCompactIndex = 7;
  req.m_mapCentreAreaCompactIndex = 99;

  auto const d = SelectFocusedArea(req);
  TEST_EQUAL(d.m_kind, FocusTargetKind::ExplorationArea, ());
  TEST(d.m_compactIndex.has_value(), ());
  TEST_EQUAL(*d.m_compactIndex, 7u, ());
}

UNIT_TEST(FocusSelection_Rule2_PanFocusesMapCentre)
{
  FocusSelectionRequest req;
  req.m_event = FocusEvent::MapPan;
  req.m_recordingActive = false;
  req.m_userAreaCompactIndex = 7;
  req.m_mapCentreAreaCompactIndex = 42;

  auto const d = SelectFocusedArea(req);
  TEST_EQUAL(d.m_kind, FocusTargetKind::ExplorationArea, ());
  TEST(d.m_compactIndex.has_value(), ());
  TEST_EQUAL(*d.m_compactIndex, 42u, ());
}

UNIT_TEST(FocusSelection_Rule1OverRule2_RecordingIgnoresPanCentre)
{
  FocusSelectionRequest req;
  req.m_event = FocusEvent::MapPan;
  req.m_recordingActive = true;
  req.m_userAreaCompactIndex = 7;
  req.m_mapCentreAreaCompactIndex = 42;

  auto const d = SelectFocusedArea(req);
  TEST_EQUAL(d.m_kind, FocusTargetKind::ExplorationArea, ());
  TEST(d.m_compactIndex.has_value(), ());
  TEST_EQUAL(*d.m_compactIndex, 7u, ());
}

UNIT_TEST(FocusSelection_Rule3_ExplicitTapFocusesSelectedArea)
{
  FocusSelectionRequest req;
  req.m_event = FocusEvent::ExplicitSelect;
  req.m_atCityScale = true;
  req.m_explicitAreaCompactIndex = 3;
  req.m_cityCompactIndex = 1;
  req.m_mapCentreAreaCompactIndex = 9;

  auto const d = SelectFocusedArea(req);
  TEST_EQUAL(d.m_kind, FocusTargetKind::ExplorationArea, ());
  TEST(d.m_compactIndex.has_value(), ());
  TEST_EQUAL(*d.m_compactIndex, 3u, ());
}

UNIT_TEST(FocusSelection_Rule4_RecentreReturnsToUserArea)
{
  FocusSelectionRequest req;
  req.m_event = FocusEvent::Recentre;
  req.m_userAreaCompactIndex = 11;
  req.m_mapCentreAreaCompactIndex = 42;

  auto const d = SelectFocusedArea(req);
  TEST_EQUAL(d.m_kind, FocusTargetKind::ExplorationArea, ());
  TEST(d.m_compactIndex.has_value(), ());
  TEST_EQUAL(*d.m_compactIndex, 11u, ());
}

UNIT_TEST(FocusSelection_Rule5_CityScaleSelectsCitySummary)
{
  TEST(IsCityScaleDrawScale(kCityScaleMaxDrawScale), ());
  TEST(IsCityScaleDrawScale(10), ());
  TEST(!IsCityScaleDrawScale(13), ());
  TEST(!IsCityScaleDrawScale(0), ());

  FocusSelectionRequest req;
  req.m_event = FocusEvent::ZoomChanged;
  req.m_atCityScale = true;
  req.m_cityCompactIndex = 1;
  req.m_userAreaCompactIndex = 7;
  req.m_mapCentreAreaCompactIndex = 7;

  auto const d = SelectFocusedArea(req);
  TEST_EQUAL(d.m_kind, FocusTargetKind::CitySummary, ());
  TEST(d.m_compactIndex.has_value(), ());
  TEST_EQUAL(*d.m_compactIndex, 1u, ());
}

UNIT_TEST(FocusSelection_NoAreaYieldsNone)
{
  FocusSelectionRequest req;
  req.m_event = FocusEvent::MapPan;
  auto const d = SelectFocusedArea(req);
  TEST_EQUAL(d.m_kind, FocusTargetKind::None, ());
  TEST(!d.m_compactIndex.has_value(), ());
}
}  // namespace
