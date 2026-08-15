package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class RoutingBuildErrorTest
{
  @Test
  public void isDownloadable_avoidExploredNoRoute_falseForMissingCount0()
  {
    assertFalse(RoutingBuildError.isDownloadable(ResultCodes.AVOID_EXPLORED_NO_ROUTE, 0));
  }

  @Test
  public void isDownloadable_avoidExploredNoRoute_falseForMissingCount1()
  {
    assertFalse(RoutingBuildError.isDownloadable(ResultCodes.AVOID_EXPLORED_NO_ROUTE, 1));
  }

  @Test
  public void isDownloadable_avoidExploredNoRoute_falseForMissingCount3()
  {
    assertFalse(RoutingBuildError.isDownloadable(ResultCodes.AVOID_EXPLORED_NO_ROUTE, 3));
  }

  @Test
  public void isDownloadable_routeNotFound_trueWhenMissingCount1()
  {
    assertTrue(RoutingBuildError.isDownloadable(ResultCodes.ROUTE_NOT_FOUND, 1));
  }

  @Test
  public void isDownloadable_routeNotFound_falseWhenMissingCount0()
  {
    assertFalse(RoutingBuildError.isDownloadable(ResultCodes.ROUTE_NOT_FOUND, 0));
  }

  @Test
  public void isDrivingOptionsBuildError_avoid_falseWhenNotRulerAndHasOptions()
  {
    assertFalse(RoutingBuildError.isDrivingOptionsBuildError(ResultCodes.AVOID_EXPLORED_NO_ROUTE, false, true));
  }

  @Test
  public void isDrivingOptionsBuildError_avoid_falseWhenNotRulerAndNoOptions()
  {
    assertFalse(RoutingBuildError.isDrivingOptionsBuildError(ResultCodes.AVOID_EXPLORED_NO_ROUTE, false, false));
  }

  @Test
  public void isDrivingOptionsBuildError_avoid_falseWhenRulerAndHasOptions()
  {
    assertFalse(RoutingBuildError.isDrivingOptionsBuildError(ResultCodes.AVOID_EXPLORED_NO_ROUTE, true, true));
  }

  @Test
  public void isDrivingOptionsBuildError_routeNotFound_trueWhenNotRulerAndHasOptions()
  {
    assertTrue(RoutingBuildError.isDrivingOptionsBuildError(ResultCodes.ROUTE_NOT_FOUND, false, true));
  }

  @Test
  public void isDrivingOptionsBuildError_needMoreMaps_falseWhenNotRulerAndHasOptions()
  {
    assertFalse(RoutingBuildError.isDrivingOptionsBuildError(ResultCodes.NEED_MORE_MAPS, false, true));
  }

  @Test
  public void isDrivingOptionsBuildError_routeNotFound_falseWhenRulerAndHasOptions()
  {
    assertFalse(RoutingBuildError.isDrivingOptionsBuildError(ResultCodes.ROUTE_NOT_FOUND, true, true));
  }
}
