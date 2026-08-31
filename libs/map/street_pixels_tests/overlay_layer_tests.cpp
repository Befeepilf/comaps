#include "testing/testing.hpp"

#include "map/street_pixels_manager.hpp"

#include "drape_frontend/exploration_area_overlay.hpp"

#include "geometry/screenbase.hpp"
#include "indexer/data_source.hpp"

namespace
{
UNIT_TEST(StreetPixelsManager_SetEnabledDoesNotEnableAreaOverlay)
{
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(!manager.IsEnabled(), ());
  TEST(!manager.IsExplorationAreasEnabled(), ());

  manager.SetEnabled(true);
  TEST(manager.IsEnabled(), ());
  TEST(!manager.IsExplorationAreasEnabled(), ());

  manager.SetExplorationAreasEnabled(true);
  TEST(manager.IsEnabled(), ());
  TEST(manager.IsExplorationAreasEnabled(), ());

  manager.SetEnabled(false);
  TEST(!manager.IsEnabled(), ());
  TEST(manager.IsExplorationAreasEnabled(), ());
}

UNIT_TEST(StreetPixelsManager_HitOverlayLabelRequiresOverlayEnabled)
{
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  ScreenBase screen;
  manager.SetEnabled(true);
  TEST(!manager.HitOverlayLabel({}, screen, 16).has_value(), ());
  manager.SetExplorationAreasEnabled(true);
  TEST(!manager.HitOverlayLabel({}, screen, 16).has_value(), ());
  manager.SetExplorationAreasEnabled(false);
  TEST(!manager.HitOverlayLabel({}, screen, 16).has_value(), ());
}

UNIT_TEST(StreetPixelsManager_OverlayZoomPrefsClamp)
{
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  StreetPixelsManager::ExplorationAreaOverlayPrefs prefs;
  prefs.m_labelMinZoom = 18;
  prefs.m_labelMaxZoom = 10;
  prefs.m_fillMinZoom = 0;
  prefs.m_fillMaxZoom = 99;
  manager.SetExplorationAreaOverlayPrefs(prefs);
  auto const got = manager.GetExplorationAreaOverlayPrefs();
  TEST_EQUAL(got.m_labelMinZoom, 18, ());
  TEST_EQUAL(got.m_labelMaxZoom, 18, ());
  TEST_EQUAL(got.m_fillMinZoom, 1, ());
  TEST_EQUAL(got.m_fillMaxZoom, 20, ());
}

UNIT_TEST(OverlayFillOpacityFactor_FadesAtRangeEdges)
{
  TEST_ALMOST_EQUAL_ABS(df::OverlayFillOpacityFactor(12.0, 9, 15), 1.0f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(df::OverlayFillOpacityFactor(9.0, 9, 15), 1.0f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(df::OverlayFillOpacityFactor(15.0, 9, 15), 1.0f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(df::OverlayFillOpacityFactor(8.0, 9, 15), 0.0f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(df::OverlayFillOpacityFactor(16.0, 9, 15), 0.0f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(df::OverlayFillOpacityFactor(8.5, 9, 15), 0.5f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(df::OverlayFillOpacityFactor(15.5, 9, 15), 0.5f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(df::OverlayFillOpacityFactor(12.0, 15, 9), 0.0f, 1e-6f, ());
}

UNIT_TEST(OverlayChromeVisible_HardCut)
{
  TEST(!df::OverlayChromeVisible(12, 13, 20), ());
  TEST(df::OverlayChromeVisible(13, 13, 20), ());
  TEST(df::OverlayChromeVisible(20, 13, 20), ());
  TEST(!df::OverlayChromeVisible(21, 13, 20), ());
}
}  // namespace
