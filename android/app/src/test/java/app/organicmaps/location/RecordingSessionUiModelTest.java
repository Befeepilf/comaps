package app.organicmaps.location;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import app.organicmaps.sdk.location.RecordingSession;
import java.util.List;
import org.junit.Test;

public class RecordingSessionUiModelTest
{
  @Test
  public void isActive_onlyRecordingAndPaused()
  {
    assertFalse(RecordingSessionUiModel.isActive(RecordingSession.STATE_IDLE));
    assertTrue(RecordingSessionUiModel.isActive(RecordingSession.STATE_RECORDING));
    assertTrue(RecordingSessionUiModel.isActive(RecordingSession.STATE_PAUSED));
    assertFalse(RecordingSessionUiModel.isActive(RecordingSession.STATE_FINISHED));
    assertFalse(RecordingSessionUiModel.isActive(RecordingSession.STATE_DISCARDED));
  }

  @Test
  public void showStatusControl_matchesActiveSession()
  {
    assertFalse(RecordingSessionUiModel.showStatusControl(RecordingSession.STATE_IDLE));
    assertTrue(RecordingSessionUiModel.showStatusControl(RecordingSession.STATE_RECORDING));
    assertTrue(RecordingSessionUiModel.showStatusControl(RecordingSession.STATE_PAUSED));
  }

  @Test
  public void pauseResumeEnablement_byState()
  {
    assertTrue(RecordingSessionUiModel.canPause(RecordingSession.STATE_RECORDING));
    assertFalse(RecordingSessionUiModel.canPause(RecordingSession.STATE_PAUSED));
    assertFalse(RecordingSessionUiModel.canPause(RecordingSession.STATE_IDLE));

    assertTrue(RecordingSessionUiModel.canResume(RecordingSession.STATE_PAUSED));
    assertFalse(RecordingSessionUiModel.canResume(RecordingSession.STATE_RECORDING));
    assertFalse(RecordingSessionUiModel.canResume(RecordingSession.STATE_IDLE));

    assertTrue(RecordingSessionUiModel.canFinishOrDiscard(RecordingSession.STATE_RECORDING));
    assertTrue(RecordingSessionUiModel.canFinishOrDiscard(RecordingSession.STATE_PAUSED));
    assertFalse(RecordingSessionUiModel.canFinishOrDiscard(RecordingSession.STATE_IDLE));
  }

  @Test
  public void statusButtonAction_pauseOrResume()
  {
    assertEquals(RecordingSessionUiModel.NotificationAction.PAUSE,
                 RecordingSessionUiModel.statusButtonAction(RecordingSession.STATE_RECORDING));
    assertEquals(RecordingSessionUiModel.NotificationAction.RESUME,
                 RecordingSessionUiModel.statusButtonAction(RecordingSession.STATE_PAUSED));
    assertNull(RecordingSessionUiModel.statusButtonAction(RecordingSession.STATE_IDLE));
  }

  @Test
  public void notificationContent_byState()
  {
    assertEquals(RecordingSessionUiModel.NotificationContent.RECORDING,
                 RecordingSessionUiModel.notificationContent(RecordingSession.STATE_RECORDING));
    assertEquals(RecordingSessionUiModel.NotificationContent.PAUSED,
                 RecordingSessionUiModel.notificationContent(RecordingSession.STATE_PAUSED));
    assertEquals(RecordingSessionUiModel.NotificationContent.IDLE,
                 RecordingSessionUiModel.notificationContent(RecordingSession.STATE_IDLE));
  }

  @Test
  public void notificationActions_recordingHasPauseAndStop()
  {
    final List<RecordingSessionUiModel.NotificationAction> actions =
        RecordingSessionUiModel.notificationActions(RecordingSession.STATE_RECORDING);
    assertEquals(2, actions.size());
    assertEquals(RecordingSessionUiModel.NotificationAction.PAUSE, actions.get(0));
    assertEquals(RecordingSessionUiModel.NotificationAction.STOP, actions.get(1));
  }

  @Test
  public void notificationActions_pausedHasResumeAndStop()
  {
    final List<RecordingSessionUiModel.NotificationAction> actions =
        RecordingSessionUiModel.notificationActions(RecordingSession.STATE_PAUSED);
    assertEquals(2, actions.size());
    assertEquals(RecordingSessionUiModel.NotificationAction.RESUME, actions.get(0));
    assertEquals(RecordingSessionUiModel.NotificationAction.STOP, actions.get(1));
  }

  @Test
  public void notificationActions_idleHasNone()
  {
    assertTrue(RecordingSessionUiModel.notificationActions(RecordingSession.STATE_IDLE).isEmpty());
  }
}
