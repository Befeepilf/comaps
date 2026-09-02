package app.organicmaps.routing;

public final class RoutePlanningUi
{
  private RoutePlanningUi() {}

  public static boolean shouldShowLayersButton(boolean requestedVisible, boolean navigating)
  {
    return requestedVisible && !navigating;
  }

  public static int layersTopMarginPx(int routingHeaderHeightPx, int framePaddingPx)
  {
    if (routingHeaderHeightPx <= 0)
      return -1;
    return routingHeaderHeightPx + Math.max(0, framePaddingPx);
  }

  public static boolean shouldStartRecordingOnRouteStart(boolean toggleEnabled, boolean sessionAlreadyActive)
  {
    return toggleEnabled && !sessionAlreadyActive;
  }
}
