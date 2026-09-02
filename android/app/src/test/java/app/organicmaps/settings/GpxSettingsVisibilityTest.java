package app.organicmaps.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class GpxSettingsVisibilityTest
{
  private static boolean enabled(boolean available, boolean entitled)
  {
    return available && entitled;
  }

  @Test
  public void showImportRow_fourCellAvailableTimesEntitled()
  {
    assertFalse(GpxSettingsVisibility.showImportRow(enabled(false, false)));
    assertFalse(GpxSettingsVisibility.showImportRow(enabled(false, true)));
    assertFalse(GpxSettingsVisibility.showImportRow(enabled(true, false)));
    assertTrue(GpxSettingsVisibility.showImportRow(enabled(true, true)));
  }

  @Test
  public void showExportRow_shownWhenEnabledRegardlessOfPro()
  {
    assertFalse(GpxSettingsVisibility.showExportRow(false));
    assertTrue(GpxSettingsVisibility.showExportRow(true));
  }

  @Test
  public void showBatchImportRow_fourCellWhenBothCapabilitiesShareTheSameFlags()
  {
    assertFalse(GpxSettingsVisibility.showBatchImportRow(enabled(false, false), enabled(false, false)));
    assertFalse(GpxSettingsVisibility.showBatchImportRow(enabled(false, true), enabled(false, true)));
    assertFalse(GpxSettingsVisibility.showBatchImportRow(enabled(true, false), enabled(true, false)));
    assertTrue(GpxSettingsVisibility.showBatchImportRow(enabled(true, true), enabled(true, true)));
  }

  @Test
  public void showBatchImportRow_hiddenWhenOnlyOneCapabilityEnabled()
  {
    assertFalse(GpxSettingsVisibility.showBatchImportRow(true, false));
    assertFalse(GpxSettingsVisibility.showBatchImportRow(false, true));
  }

  @Test
  public void showInfoPage_hiddenWhenNoCapabilityAvailable()
  {
    assertFalse(GpxSettingsVisibility.showInfoPage(false, false, false));
  }

  @Test
  public void showInfoPage_shownWhenAvailableAndNotEntitled()
  {
    assertTrue(GpxSettingsVisibility.showInfoPage(true, false, false));
  }

  @Test
  public void showInfoPage_shownWhenAvailableAndEntitled()
  {
    assertTrue(GpxSettingsVisibility.showInfoPage(true, true, true));
  }

  @Test
  public void showInfoPage_shownWhenAnyCapabilityAvailable()
  {
    assertTrue(GpxSettingsVisibility.showInfoPage(true, false, false));
    assertTrue(GpxSettingsVisibility.showInfoPage(false, true, false));
    assertTrue(GpxSettingsVisibility.showInfoPage(false, false, true));
  }

  @Test
  public void showGpxScreen_hiddenWhenNoRows()
  {
    assertFalse(GpxSettingsVisibility.showGpxScreen(false, false, false, false));
  }

  @Test
  public void showGpxScreen_shownWhenOnlyExport()
  {
    assertTrue(GpxSettingsVisibility.showGpxScreen(false, true, false, false));
  }

  @Test
  public void showGpxScreen_shownWhenOnlyInfo()
  {
    assertTrue(GpxSettingsVisibility.showGpxScreen(false, false, false, true));
  }

  @Test
  public void showGpxScreen_shownWhenOnlyImport()
  {
    assertTrue(GpxSettingsVisibility.showGpxScreen(true, false, false, false));
  }

  @Test
  public void exportEnabledDoesNotShowImportRow()
  {
    assertFalse(GpxSettingsVisibility.showImportRow(false));
    assertTrue(GpxSettingsVisibility.showExportRow(true));
  }
}
