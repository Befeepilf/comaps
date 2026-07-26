package app.organicmaps.sdk.location;

import androidx.annotation.IntDef;

import app.organicmaps.sdk.BuildConfig;
import app.organicmaps.sdk.Framework;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public final class RecordingSessionDebug
{
  public static final int STATE_IDLE = 0;
  public static final int STATE_RECORDING = 1;
  public static final int STATE_PAUSED = 2;
  public static final int STATE_FINISHED = 3;
  public static final int STATE_DISCARDED = 4;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ STATE_IDLE, STATE_RECORDING, STATE_PAUSED, STATE_FINISHED, STATE_DISCARDED })
  public @interface State {}

  private RecordingSessionDebug() {}

  public static void start()
  {
    if (!BuildConfig.DEBUG)
      return;
    Framework.nativeRecordingSessionStart();
  }

  public static void pause()
  {
    if (!BuildConfig.DEBUG)
      return;
    Framework.nativeRecordingSessionPause();
  }

  public static void resume()
  {
    if (!BuildConfig.DEBUG)
      return;
    Framework.nativeRecordingSessionResume();
  }

  public static void finish()
  {
    if (!BuildConfig.DEBUG)
      return;
    Framework.nativeRecordingSessionFinish();
  }

  public static void discard()
  {
    if (!BuildConfig.DEBUG)
      return;
    Framework.nativeRecordingSessionDiscard();
  }

  @State
  public static int getState()
  {
    if (!BuildConfig.DEBUG)
      return STATE_IDLE;
    return Framework.nativeRecordingSessionGetState();
  }
}
