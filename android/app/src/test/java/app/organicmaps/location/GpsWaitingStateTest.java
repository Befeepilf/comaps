package app.organicmaps.location;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class GpsWaitingStateTest
{
  @Test
  public void showWaiting_hiddenWhenIdle()
  {
    assertFalse(GpsWaitingState.showWaiting(false, false, false, false, 40.0f));
  }

  @Test
  public void showWaiting_hiddenWhenPaused()
  {
    assertFalse(GpsWaitingState.showWaiting(true, true, true, true, 40.0f));
  }

  @Test
  public void showWaiting_shownWhenRecordingWithoutFix()
  {
    assertTrue(GpsWaitingState.showWaiting(true, false, false, false, 0.0f));
  }

  @Test
  public void showWaiting_shownWhenLocationHasNoAccuracy()
  {
    assertTrue(GpsWaitingState.showWaiting(true, false, true, false, 0.0f));
  }

  @Test
  public void showWaiting_shownJustOutsideAccuracyLimit()
  {
    assertTrue(GpsWaitingState.showWaiting(true, false, true, true, 25.01f));
  }

  @Test
  public void showWaiting_hiddenAtAccuracyLimit()
  {
    assertFalse(GpsWaitingState.showWaiting(true, false, true, true, 25.0f));
  }

  @Test
  public void showWaiting_hiddenWhenAccurate()
  {
    assertFalse(GpsWaitingState.showWaiting(true, false, true, true, 8.0f));
  }
}
