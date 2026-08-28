package app.organicmaps.sdk;

public final class ExplorerPro
{
  private static boolean sNativeReady = false;

  private ExplorerPro() {}

  static void setNativeReady(boolean ready)
  {
    sNativeReady = ready;
  }

  static boolean combine(boolean nativeReady, boolean nativeEnabled)
  {
    return nativeReady && nativeEnabled;
  }

  public static boolean isGpxImportEnabled()
  {
    return combine(sNativeReady, sNativeReady && Framework.nativeIsGpxImportEnabled());
  }

  public static boolean isGpxExportEnabled()
  {
    return combine(sNativeReady, sNativeReady && Framework.nativeIsGpxExportEnabled());
  }

  public static boolean isAdvancedTrackManagementEnabled()
  {
    return combine(sNativeReady, sNativeReady && Framework.nativeIsAdvancedTrackManagementEnabled());
  }

  public static boolean isGpxImportAvailable()
  {
    return combine(sNativeReady, sNativeReady && Framework.nativeIsGpxImportAvailable());
  }

  public static boolean isGpxExportAvailable()
  {
    return combine(sNativeReady, sNativeReady && Framework.nativeIsGpxExportAvailable());
  }

  public static boolean isAdvancedTrackManagementAvailable()
  {
    return combine(sNativeReady, sNativeReady && Framework.nativeIsAdvancedTrackManagementAvailable());
  }
}
