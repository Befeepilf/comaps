package app.organicmaps.sdk.routing;

public final class RoutingBuildError
{
  private RoutingBuildError() {}

  public static boolean isDownloadable(int resultCode, int missingCount)
  {
    if (missingCount <= 0)
      return false;

    return switch (resultCode)
    {
      case ResultCodes.INCONSISTENT_MWM_ROUTE, ResultCodes.ROUTE_NOT_FOUND_REDRESS_ROUTE_ERROR,
          ResultCodes.ROUTING_FILE_NOT_EXIST, ResultCodes.NEED_MORE_MAPS, ResultCodes.ROUTE_NOT_FOUND,
          ResultCodes.FILE_TOO_OLD ->
        true;
      default -> false;
    };
  }

  public static boolean isDrivingOptionsBuildError(int resultCode, boolean isRulerRouter, boolean hasAnyOptions)
  {
    return resultCode != ResultCodes.NEED_MORE_MAPS
        && resultCode != ResultCodes.AVOID_EXPLORED_NO_ROUTE
        && !isRulerRouter
        && hasAnyOptions;
  }
}
