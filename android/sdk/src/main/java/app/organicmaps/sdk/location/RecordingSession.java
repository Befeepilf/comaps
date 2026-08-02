package app.organicmaps.sdk.location;

import androidx.annotation.IntDef;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.sdk.Framework;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

public final class RecordingSession
{
  public static final int STATE_IDLE = 0;
  public static final int STATE_RECORDING = 1;
  public static final int STATE_PAUSED = 2;
  public static final int STATE_FINISHED = 3;
  public static final int STATE_DISCARDED = 4;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({STATE_IDLE, STATE_RECORDING, STATE_PAUSED, STATE_FINISHED, STATE_DISCARDED})
  public @interface State
  {}

  public interface StateListener
  {
    void onStateChanged(@State int previous, @State int current);
  }

  @NonNull
  private static final List<StateListener> sListeners = new ArrayList<>();

  @NonNull
  private static final OnRecordingSessionChangedListener sJniListener = new OnRecordingSessionChangedListener();

  private static boolean sJniListenerRegistered = false;

  private RecordingSession() {}

  public static void start()
  {
    Framework.nativeRecordingSessionStart();
  }

  public static void pause()
  {
    Framework.nativeRecordingSessionPause();
  }

  public static void resume()
  {
    Framework.nativeRecordingSessionResume();
  }

  public static void finish()
  {
    Framework.nativeRecordingSessionFinish();
  }

  public static void discard()
  {
    Framework.nativeRecordingSessionDiscard();
  }

  public static void reset()
  {
    Framework.nativeRecordingSessionReset();
  }

  @State
  public static int getState()
  {
    return Framework.nativeRecordingSessionGetState();
  }

  public static boolean isActive(@State int state)
  {
    return state == STATE_RECORDING || state == STATE_PAUSED;
  }

  public static boolean isActive()
  {
    return isActive(getState());
  }

  public static void registerListener(@NonNull StateListener listener)
  {
    synchronized (sListeners)
    {
      if (!sListeners.contains(listener))
        sListeners.add(listener);
      ensureJniListenerRegistered();
    }
  }

  public static void unregisterListener(@NonNull StateListener listener)
  {
    synchronized (sListeners)
    {
      sListeners.remove(listener);
      if (sListeners.isEmpty() && sJniListenerRegistered)
      {
        nativeRemoveListener();
        sJniListenerRegistered = false;
      }
    }
  }

  static void notifyListeners(@State int previous, @State int current)
  {
    final List<StateListener> snapshot;
    synchronized (sListeners)
    {
      snapshot = new ArrayList<>(sListeners);
    }
    for (StateListener listener : snapshot)
      listener.onStateChanged(previous, current);
  }

  private static void ensureJniListenerRegistered()
  {
    if (sJniListenerRegistered)
      return;
    nativeAddListener(sJniListener);
    sJniListenerRegistered = true;
  }

  private static native void nativeAddListener(@NonNull OnRecordingSessionChangedListener listener);

  private static native void nativeRemoveListener();

  private static final class OnRecordingSessionChangedListener
  {
    @Keep
    @SuppressWarnings("unused")
    public void onStateChanged(int previous, int current)
    {
      notifyListeners(previous, current);
    }
  }
}
