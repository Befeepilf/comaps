package app.organicmaps.maplayer;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import app.organicmaps.sdk.maplayer.streetpixels.AreaMilestonePresentation;
import org.junit.Test;

public class CompletionCardGeneratedGateTest
{
  @Test
  public void shouldRecord_oncePerHundredPercentArea()
  {
    assertTrue(CompletionCardGeneratedGate.shouldRecord(false, AreaMilestonePresentation.THRESHOLD_100, 11L, 0L));
    assertFalse(CompletionCardGeneratedGate.shouldRecord(false, AreaMilestonePresentation.THRESHOLD_100, 11L, 11L));
    assertTrue(CompletionCardGeneratedGate.shouldRecord(false, AreaMilestonePresentation.THRESHOLD_100, 22L, 11L));
  }

  @Test
  public void shouldRecord_skippedForDebugAndNonHundred()
  {
    assertFalse(CompletionCardGeneratedGate.shouldRecord(true, AreaMilestonePresentation.THRESHOLD_100, 11L, 0L));
    assertFalse(CompletionCardGeneratedGate.shouldRecord(false, AreaMilestonePresentation.THRESHOLD_50, 11L, 0L));
    assertFalse(CompletionCardGeneratedGate.shouldRecord(false, AreaMilestonePresentation.THRESHOLD_100, 0L, 0L));
  }
}
