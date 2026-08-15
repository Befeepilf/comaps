#include "testing/testing.hpp"

#include "street_pixels_areas/exploration_area_tap.hpp"

UNIT_TEST(ClassifyMapTap_PointPoiIsDiscreteObject)
{
  street_pixels::MapTapClassification tap;
  tap.m_isPointFeature = true;
  TEST_EQUAL(street_pixels::ClassifyMapTap(tap), street_pixels::MapTapKind::DiscreteObject, ());
}

UNIT_TEST(ClassifyMapTap_BookmarkIsDiscreteObject)
{
  street_pixels::MapTapClassification tap;
  tap.m_isBookmark = true;
  TEST_EQUAL(street_pixels::ClassifyMapTap(tap), street_pixels::MapTapKind::DiscreteObject, ());
}

UNIT_TEST(ClassifyMapTap_MyPositionIsDiscreteObject)
{
  street_pixels::MapTapClassification tap;
  tap.m_isMyPosition = true;
  TEST_EQUAL(street_pixels::ClassifyMapTap(tap), street_pixels::MapTapKind::DiscreteObject, ());
}

UNIT_TEST(ClassifyMapTap_TrackIsDiscreteObject)
{
  street_pixels::MapTapClassification tap;
  tap.m_isTrack = true;
  TEST_EQUAL(street_pixels::ClassifyMapTap(tap), street_pixels::MapTapKind::DiscreteObject, ());
}

UNIT_TEST(ClassifyMapTap_RoutePointIsDiscreteObject)
{
  street_pixels::MapTapClassification tap;
  tap.m_isRoutePoint = true;
  TEST_EQUAL(street_pixels::ClassifyMapTap(tap), street_pixels::MapTapKind::DiscreteObject, ());
}

UNIT_TEST(ClassifyMapTap_EmptyMapIsAreaSurface)
{
  street_pixels::MapTapClassification tap;
  TEST_EQUAL(street_pixels::ClassifyMapTap(tap), street_pixels::MapTapKind::AreaSurface, ());
}

UNIT_TEST(ClassifyMapTap_BuildingOrLanduseWithoutPointGeomIsAreaSurface)
{
  street_pixels::MapTapClassification tap;
  TEST_EQUAL(street_pixels::ClassifyMapTap(tap), street_pixels::MapTapKind::AreaSurface, ());
}

UNIT_TEST(ClassifyMapTap_PlaceLabelIsAreaLabelEvenIfPoint)
{
  street_pixels::MapTapClassification tap;
  tap.m_isPointFeature = true;
  tap.m_isAreaLabel = true;
  TEST_EQUAL(street_pixels::ClassifyMapTap(tap), street_pixels::MapTapKind::AreaLabel, ());
}

UNIT_TEST(ClassifyMapTap_PlaceAreaGeomIsAreaLabel)
{
  street_pixels::MapTapClassification tap;
  tap.m_isAreaLabel = true;
  TEST_EQUAL(street_pixels::ClassifyMapTap(tap), street_pixels::MapTapKind::AreaLabel, ());
}

UNIT_TEST(ClassifyMapTap_BookmarkWinsOverAreaLabel)
{
  street_pixels::MapTapClassification tap;
  tap.m_isBookmark = true;
  tap.m_isAreaLabel = true;
  TEST_EQUAL(street_pixels::ClassifyMapTap(tap), street_pixels::MapTapKind::DiscreteObject, ());
}

UNIT_TEST(ShouldOpenExplorationDetail_PointPoiNeverOpensEvenInsideArea)
{
  TEST(!street_pixels::ShouldOpenExplorationDetail(street_pixels::MapTapKind::DiscreteObject, true), ());
}

UNIT_TEST(ShouldOpenExplorationDetail_BuildingInsideAreaDoesNotOpen)
{
  TEST(!street_pixels::ShouldOpenExplorationDetail(street_pixels::MapTapKind::AreaSurface, true), ());
}

UNIT_TEST(ShouldOpenExplorationDetail_EmptyMapDoesNotOpen)
{
  TEST(!street_pixels::ShouldOpenExplorationDetail(street_pixels::MapTapKind::AreaSurface, true), ());
  TEST(!street_pixels::ShouldOpenExplorationDetail(street_pixels::MapTapKind::AreaSurface, false), ());
}

UNIT_TEST(ShouldOpenExplorationDetail_AreaLabelHitOpens)
{
  TEST(street_pixels::ShouldOpenExplorationDetail(street_pixels::MapTapKind::AreaLabel, true), ());
}

UNIT_TEST(ShouldOpenExplorationDetail_AreaLabelMissFallsThroughToPlacePage)
{
  TEST(!street_pixels::ShouldOpenExplorationDetail(street_pixels::MapTapKind::AreaLabel, false), ());
}
