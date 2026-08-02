package app.organicmaps.location;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.sdk.location.RecordingSession;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public final class RecordingSessionUiModel
{
  public enum NotificationAction
  {
    PAUSE,
    RESUME,
    STOP
  }

  public enum NotificationContent
  {
    IDLE,
    RECORDING,
    PAUSED
  }

  private RecordingSessionUiModel() {}

  public static boolean isActive(@RecordingSession.State int state)
  {
    return RecordingSession.isActive(state);
  }

  public static boolean showStatusControl(@RecordingSession.State int state)
  {
    return isActive(state);
  }

  public static boolean canPause(@RecordingSession.State int state)
  {
    return state == RecordingSession.STATE_RECORDING;
  }

  public static boolean canResume(@RecordingSession.State int state)
  {
    return state == RecordingSession.STATE_PAUSED;
  }

  public static boolean canFinishOrDiscard(@RecordingSession.State int state)
  {
    return isActive(state);
  }

  @Nullable
  public static NotificationAction statusButtonAction(@RecordingSession.State int state)
  {
    if (canPause(state))
      return NotificationAction.PAUSE;
    if (canResume(state))
      return NotificationAction.RESUME;
    return null;
  }

  @NonNull
  public static NotificationContent notificationContent(@RecordingSession.State int state)
  {
    if (state == RecordingSession.STATE_PAUSED)
      return NotificationContent.PAUSED;
    if (state == RecordingSession.STATE_RECORDING)
      return NotificationContent.RECORDING;
    return NotificationContent.IDLE;
  }

  @NonNull
  public static List<NotificationAction> notificationActions(@RecordingSession.State int state)
  {
    if (!isActive(state))
      return Collections.emptyList();

    final List<NotificationAction> actions = new ArrayList<>(2);
    if (canPause(state))
      actions.add(NotificationAction.PAUSE);
    else if (canResume(state))
      actions.add(NotificationAction.RESUME);
    actions.add(NotificationAction.STOP);
    return actions;
  }
}
