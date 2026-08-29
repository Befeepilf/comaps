package app.organicmaps.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class FirstRunFlowTest
{
  @Test
  public void requestLocationOnAppOpen_never()
  {
    assertFalse(FirstRunFlow.requestLocationOnAppOpen());
  }

  @Test
  public void shouldShowExploringCard_onlyWhenUnseen()
  {
    assertTrue(FirstRunFlow.shouldShowExploringCard(false));
    assertFalse(FirstRunFlow.shouldShowExploringCard(true));
  }

  @Test
  public void bundleCompetitionWithLocationRationale_never()
  {
    assertFalse(FirstRunFlow.bundleCompetitionWithLocationRationale());
  }
}
