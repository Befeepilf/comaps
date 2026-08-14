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

UNIT_TEST(ShouldOpenExplorationDetail_PointPoiNeverOpensEvenInsideArea)
{
  TEST(!street_pixels::ShouldOpenExplorationDetail(street_pixels::MapTapKind::DiscreteObject, true, true), ());
}

UNIT_TEST(ShouldOpenExplorationDetail_BuildingInsideAreaOpens)
{
  TEST(street_pixels::ShouldOpenExplorationDetail(street_pixels::MapTapKind::AreaSurface, true, true), ());
}

UNIT_TEST(ShouldOpenExplorationDetail_EmptyMapInsideOrOutsideOpens)
{
  TEST(street_pixels::ShouldOpenExplorationDetail(street_pixels::MapTapKind::AreaSurface, true, false), ());
  TEST(street_pixels::ShouldOpenExplorationDetail(street_pixels::MapTapKind::AreaSurface, false, false), ());
}

UNIT_TEST(ShouldOpenExplorationDetail_BuildingOutsideAreaFallsThroughToPlacePage)
{
  TEST(!street_pixels::ShouldOpenExplorationDetail(street_pixels::MapTapKind::AreaSurface, false, true), ());
}
