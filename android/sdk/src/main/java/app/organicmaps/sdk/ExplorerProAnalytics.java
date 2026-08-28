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
    return nativeGetInfoPageViewed();
  }

  public static long getGpxImportUsage()
  {
    return nativeGetGpxImportUsage();
  }

  public static long getGpxExportUsage()
  {
    return nativeGetGpxExportUsage();
  }

  private static native void nativeRecordInfoPageViewed();
  private static native long nativeGetInfoPageViewed();
  private static native long nativeGetGpxImportUsage();
  private static native long nativeGetGpxExportUsage();
}
