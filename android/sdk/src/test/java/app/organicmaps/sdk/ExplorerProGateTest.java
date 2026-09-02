package app.organicmaps.sdk;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.After;
import org.junit.Test;

public class ExplorerProGateTest
{
  @After
  public void resetNativeReady()
  {
    ExplorerPro.setNativeReady(false);
  }

  @Test
  public void combine_closedWhenNativeNotReady()
  {
    assertFalse(ExplorerPro.combine(false, true));
  }

  @Test
  public void combine_closedWhenNativeDisabled()
  {
    assertFalse(ExplorerPro.combine(true, false));
  }

  @Test
  public void combine_closedWhenBothFalse()
  {
    assertFalse(ExplorerPro.combine(false, false));
  }

  @Test
  public void combine_openWhenReadyAndEnabled()
  {
    assertTrue(ExplorerPro.combine(true, true));
  }

  @Test
  public void isGpxImportEnabled_closedWhenNativeNotReady()
  {
    ExplorerPro.setNativeReady(false);
    assertFalse(ExplorerPro.isGpxImportEnabled());
  }

  @Test
  public void isGpxExportEnabled_closedWhenNativeNotReady()
  {
    ExplorerPro.setNativeReady(false);
    assertFalse(ExplorerPro.isGpxExportEnabled());
  }

  @Test
  public void isGpxExportEnabled_openWhenNativeReadyRegardlessOfPro()
  {
    ExplorerPro.setNativeReady(true);
    assertTrue(ExplorerPro.isGpxExportEnabled());
  }

  @Test
  public void isAdvancedTrackManagementEnabled_closedWhenNativeNotReady()
  {
    ExplorerPro.setNativeReady(false);
    assertFalse(ExplorerPro.isAdvancedTrackManagementEnabled());
  }

  @Test
  public void isGpxImportAvailable_closedWhenNativeNotReady()
  {
    ExplorerPro.setNativeReady(false);
    assertFalse(ExplorerPro.isGpxImportAvailable());
  }

  @Test
  public void isGpxExportAvailable_closedWhenNativeNotReady()
  {
    ExplorerPro.setNativeReady(false);
    assertFalse(ExplorerPro.isGpxExportAvailable());
  }

  @Test
  public void isAdvancedTrackManagementAvailable_closedWhenNativeNotReady()
  {
    ExplorerPro.setNativeReady(false);
    assertFalse(ExplorerPro.isAdvancedTrackManagementAvailable());
  }

  @Test
  public void isNativeReady_closedWhenNativeNotReady()
  {
    ExplorerPro.setNativeReady(false);
    assertFalse(ExplorerPro.isNativeReady());
  }
}
