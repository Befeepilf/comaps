package app.organicmaps.sdk.location;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public final class RecordingSessionDebug
{
  public static final int STATE_IDLE = RecordingSession.STATE_IDLE;
  public static final int STATE_RECORDING = RecordingSession.STATE_RECORDING;
  public static final int STATE_PAUSED = RecordingSession.STATE_PAUSED;
  public static final int STATE_FINISHED = RecordingSession.STATE_FINISHED;
  public static final int STATE_DISCARDED = RecordingSession.STATE_DISCARDED;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({STATE_IDLE, STATE_RECORDING, STATE_PAUSED, STATE_FINISHED, STATE_DISCARDED})
  public @interface State
  {}

  private RecordingSessionDebug() {}

  public static void start()
  {
    RecordingSession.start();
  }

  public static void pause()
  {
    RecordingSession.pause();
  }

  public static void resume()
  {
    RecordingSession.resume();
  }

  public static void finish()
  {
    RecordingSession.finish();
  }

  public static void discard()
  {
    RecordingSession.discard();
  }

  public static void reset()
  {
    RecordingSession.reset();
  }

  @State
  public static int getState()
  {
    return RecordingSession.getState();
  }
}
