package app.organicmaps.sdk;

public final class ExplorerProAnalytics
{
  private ExplorerProAnalytics() {}

  public static void recordInfoPageViewed()
  {
    if (!ExplorerPro.isGpxImportAvailable() && !ExplorerPro.isGpxExportAvailable()
        && !ExplorerPro.isAdvancedTrackManagementAvailable())
      return;
    nativeRecordInfoPageViewed();
  }

  public static long getInfoPageViewed()
  {
    if (!ExplorerPro.isNativeReady())
      return 0;
    return nativeGetInfoPageViewed();
  }

  public static long getGpxImportUsage()
  {
    if (!ExplorerPro.isNativeReady())
      return 0;
    return nativeGetGpxImportUsage();
  }

  public static long getGpxExportUsage()
  {
    if (!ExplorerPro.isNativeReady())
      return 0;
    return nativeGetGpxExportUsage();
  }

  private static native void nativeRecordInfoPageViewed();
  private static native long nativeGetInfoPageViewed();
  private static native long nativeGetGpxImportUsage();
  private static native long nativeGetGpxExportUsage();
}
