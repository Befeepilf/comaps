package app.organicmaps.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class IncompleteSpaSettingsVisibilityTest
{
  @Test
  public void showRow_hiddenWhenNoneIncomplete()
  {
    assertFalse(IncompleteSpaSettingsVisibility.showRow(0));
    assertFalse(IncompleteSpaSettingsVisibility.showRow(-1));
  }

  @Test
  public void showRow_shownWhenAnyIncomplete()
  {
    assertTrue(IncompleteSpaSettingsVisibility.showRow(1));
    assertTrue(IncompleteSpaSettingsVisibility.showRow(3));
  }
}
