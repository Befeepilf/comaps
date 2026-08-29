package app.organicmaps.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertEquals;

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

  @Test
  public void startExploringResult_survivesWithoutListenerField()
  {
    assertEquals("first_run_start_exploring", FirstRunExploringDialogFragment.RESULT_START_EXPLORING);
  }
}
