package app.organicmaps.settings;

public final class GpxSettingsVisibility
{
  private GpxSettingsVisibility() {}

  public static boolean showImportRow(boolean gpxImportEnabled)
  {
    return gpxImportEnabled;
  }

  public static boolean showExportRow(boolean gpxExportEnabled)
  {
    return gpxExportEnabled;
  }

  public static boolean showBatchImportRow(boolean gpxImportEnabled, boolean advancedTrackManagementEnabled)
  {
    return gpxImportEnabled && advancedTrackManagementEnabled;
  }

  public static boolean showInfoPage(boolean gpxImportAvailable, boolean gpxExportAvailable,
                                     boolean advancedTrackManagementAvailable)
  {
    return gpxImportAvailable || gpxExportAvailable || advancedTrackManagementAvailable;
  }

  public static boolean showGpxScreen(boolean showImport, boolean showExport, boolean showBatch, boolean showInfo)
  {
    return showImport || showExport || showBatch || showInfo;
  }
}
