package app.organicmaps.sdk;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class ExplorerProGateTest
{
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
}
