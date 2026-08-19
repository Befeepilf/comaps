package app.organicmaps.sdk.maplayer.streetpixels;

import android.app.Application;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

class OnStreetPixelsChangedListener
{
  private StreetPixelsErrorDialogListener mListener;

  OnStreetPixelsChangedListener()
  {}

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public void onStateChanged(boolean enabled, int status, @NonNull String countryId)
  {
    StreetPixelsState.Status newStatus = StreetPixelsState.Status.values()[status];
    StreetPixelsState state = new StreetPixelsState(enabled, newStatus, countryId);
    StreetPixelsManager.updateState(state);
    if (mListener == null)
      return;

    mListener.onStateChanged(state);
  }

  @Keep
  @SuppressWarnings("unused")
  public void onFocusedAreaProgressChanged(@NonNull FocusedAreaProgress progress)
  {
    StreetPixelsManager.notifyFocusedAreaProgress(progress);
  }

  @Keep
  @SuppressWarnings("unused")
  public void onExplorationAreaTapped(@NonNull FocusedAreaProgress progress)
  {
    StreetPixelsManager.notifyExplorationAreaTapped(progress);
  }

  @Keep
  @SuppressWarnings("unused")
  public void onFirstGoalProgressChanged(@NonNull FirstGoalProgress progress)
  {
    StreetPixelsManager.notifyFirstGoalProgress(progress);
  }

  public void attach(@NonNull StreetPixelsErrorDialogListener listener)
  {
    mListener = listener;
  }

  public void detach()
  {
    mListener = null;
  }
}