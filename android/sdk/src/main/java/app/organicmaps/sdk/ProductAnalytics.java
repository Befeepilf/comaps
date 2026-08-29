package app.organicmaps.sdk;

public final class ProductAnalytics
{
  private ProductAnalytics() {}

  public static void recordPositionPermissionGranted()
  {
    if (!ExplorerPro.isNativeReady())
      return;
    nativeRecordPositionPermissionGranted();
  }

  public static void recordNotifyPermissionGranted()
  {
    if (!ExplorerPro.isNativeReady())
      return;
    nativeRecordNotifyPermissionGranted();
  }

  public static void recordCompetitionPromptViewed()
  {
    if (!ExplorerPro.isNativeReady())
      return;
    nativeRecordCompetitionPromptViewed();
  }

  private static native void nativeRecordPositionPermissionGranted();
  private static native void nativeRecordNotifyPermissionGranted();
  private static native void nativeRecordCompetitionPromptViewed();
}
