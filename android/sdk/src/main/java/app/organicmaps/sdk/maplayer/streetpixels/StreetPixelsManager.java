package app.organicmaps.sdk.maplayer.streetpixels;

import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.sdk.Framework;

public class StreetPixelsManager
{
  private static volatile StreetPixelsState.Status sStatus = StreetPixelsState.Status.NOT_READY;

  @NonNull
  private final OnStreetPixelsChangedListener mListener;

  public interface Callback
  {
    void onStateChanged(boolean enabled, @NonNull StreetPixelsState.Status status, @NonNull String countryId);
  }

  public interface FocusedAreaProgressCallback
  {
    void onFocusedAreaProgressChanged(@NonNull FocusedAreaProgress progress);
  }

  public interface ExplorationAreaTapCallback
  {
    void onExplorationAreaTapped(@NonNull FocusedAreaProgress progress);
  }

  public interface FirstGoalProgressCallback
  {
    void onFirstGoalProgressChanged(@NonNull FirstGoalProgress progress);
  }

  public interface AreaMilestonePresentationCallback
  {
    void onAreaMilestonePresentationChanged(@Nullable AreaMilestonePresentation presentation);
  }

  @NonNull
  private static final java.util.List<Callback> sCallbacks = new java.util.ArrayList<>();
  @NonNull
  private static final java.util.List<FocusedAreaProgressCallback> sProgressCallbacks = new java.util.ArrayList<>();
  @NonNull
  private static final java.util.List<ExplorationAreaTapCallback> sTapCallbacks = new java.util.ArrayList<>();
  @NonNull
  private static final java.util.List<FirstGoalProgressCallback> sFirstGoalCallbacks = new java.util.ArrayList<>();
  @NonNull
  private static final java.util.List<AreaMilestonePresentationCallback> sAreaMilestoneCallbacks =
      new java.util.ArrayList<>();

  public static void registerCallback(@NonNull Callback callback)
  {
    synchronized (sCallbacks)
    {
      if (!sCallbacks.contains(callback))
        sCallbacks.add(callback);
    }
  }

  public static void unregisterCallback(@NonNull Callback callback)
  {
    synchronized (sCallbacks)
    {
      sCallbacks.remove(callback);
    }
  }

  public static void registerFocusedAreaProgressCallback(@NonNull FocusedAreaProgressCallback callback)
  {
    synchronized (sProgressCallbacks)
    {
      if (!sProgressCallbacks.contains(callback))
        sProgressCallbacks.add(callback);
    }
  }

  public static void unregisterFocusedAreaProgressCallback(@NonNull FocusedAreaProgressCallback callback)
  {
    synchronized (sProgressCallbacks)
    {
      sProgressCallbacks.remove(callback);
    }
  }

  public static void registerExplorationAreaTapCallback(@NonNull ExplorationAreaTapCallback callback)
  {
    synchronized (sTapCallbacks)
    {
      if (!sTapCallbacks.contains(callback))
        sTapCallbacks.add(callback);
    }
  }

  public static void unregisterExplorationAreaTapCallback(@NonNull ExplorationAreaTapCallback callback)
  {
    synchronized (sTapCallbacks)
    {
      sTapCallbacks.remove(callback);
    }
  }

  public static void registerFirstGoalProgressCallback(@NonNull FirstGoalProgressCallback callback)
  {
    synchronized (sFirstGoalCallbacks)
    {
      if (!sFirstGoalCallbacks.contains(callback))
        sFirstGoalCallbacks.add(callback);
    }
  }

  public static void unregisterFirstGoalProgressCallback(@NonNull FirstGoalProgressCallback callback)
  {
    synchronized (sFirstGoalCallbacks)
    {
      sFirstGoalCallbacks.remove(callback);
    }
  }

  public static void registerAreaMilestonePresentationCallback(@NonNull AreaMilestonePresentationCallback callback)
  {
    synchronized (sAreaMilestoneCallbacks)
    {
      if (!sAreaMilestoneCallbacks.contains(callback))
        sAreaMilestoneCallbacks.add(callback);
    }
  }

  public static void unregisterAreaMilestonePresentationCallback(@NonNull AreaMilestonePresentationCallback callback)
  {
    synchronized (sAreaMilestoneCallbacks)
    {
      sAreaMilestoneCallbacks.remove(callback);
    }
  }

  public StreetPixelsManager()
  {
    mListener = new OnStreetPixelsChangedListener();
  }

  static public boolean isEnabled()
  {
    return Framework.nativeIsStreetPixelsLayerEnabled();
  }

  private void registerListener()
  {
    nativeAddListener(mListener);
  }

  static public void setEnabled(boolean isEnabled)
  {
    if (isEnabled == isEnabled())
      return;

    Framework.nativeSetStreetPixelsLayerEnabled(isEnabled);
  }

  public void initialize()
  {
    registerListener();
  }

  private static native void nativeAddListener(@NonNull OnStreetPixelsChangedListener listener);
  private static native void nativeRemoveListener(@NonNull OnStreetPixelsChangedListener listener);
  private static native boolean nativeShouldShowNotification();
  private static native double nativeGetTotalExploredFraction();
  @Nullable
  private static native RematchFractionChange nativeTakePendingRematchFractionChange(@NonNull String countryId);
  @NonNull
  private static native FocusedAreaProgress nativeGetFocusedAreaProgress();
  @NonNull
  private static native FocusedAreaProgress nativeRefreshFocusedAreaAtMapCenter(@NonNull String countryId);
  @NonNull
  private static native FocusedAreaProgress nativeSelectFocusedAreaAtLatLon(double lat, double lon,
                                                                            @NonNull String countryId);
  @NonNull
  private static native FirstGoalProgress nativeGetFirstGoalProgress();
  @Nullable
  private static native AreaMilestonePresentation nativeGetCurrentAreaMilestonePresentation();
  private static native void nativeAcknowledgeAreaMilestonePresentation();

  public void attach(@NonNull StreetPixelsErrorDialogListener listener)
  {
    mListener.attach(listener);
  }

  public void detach()
  {
    mListener.detach();
  }

  public boolean shouldShowNotification()
  {
    return nativeShouldShowNotification();
  }

  public double getTotalExploredFraction()
  {
    return nativeGetTotalExploredFraction();
  }

  @NonNull
  public FocusedAreaProgress getFocusedAreaProgress()
  {
    return nativeGetFocusedAreaProgress();
  }

  // Refresh focus via §12.5 engine (recording / follow / pan / city zoom).
  @NonNull
  public FocusedAreaProgress refreshFocusedAreaAtMapCenter(@NonNull String countryId)
  {
    return nativeRefreshFocusedAreaAtMapCenter(countryId);
  }

  // Polygon hit-test tap → §12.5 rule 3 explicit focus (SP-038).
  @NonNull
  public FocusedAreaProgress selectFocusedAreaAtLatLon(double lat, double lon, @NonNull String countryId)
  {
    return nativeSelectFocusedAreaAtLatLon(lat, lon, countryId);
  }

  @NonNull
  public FirstGoalProgress getFirstGoalProgress()
  {
    return nativeGetFirstGoalProgress();
  }

  @Nullable
  public AreaMilestonePresentation getCurrentAreaMilestonePresentation()
  {
    return nativeGetCurrentAreaMilestonePresentation();
  }

  public void acknowledgeAreaMilestonePresentation()
  {
    nativeAcknowledgeAreaMilestonePresentation();
  }

  @Nullable
  public RematchFractionChange takePendingRematchFractionChange(@NonNull String countryId)
  {
    return nativeTakePendingRematchFractionChange(countryId);
  }

  public static boolean isLoading()
  {
    return sStatus == StreetPixelsState.Status.LOADING;
  }

  public static void updateState(@NonNull StreetPixelsState state)
  {
    sStatus = state.getStatus();
    java.util.List<Callback> snapshot;
    synchronized (sCallbacks)
    {
      snapshot = new java.util.ArrayList<>(sCallbacks);
    }
    for (Callback cb : snapshot)
    {
      cb.onStateChanged(state.isEnabled(), state.getStatus(), state.getCountryId());
    }
  }

  static void notifyFocusedAreaProgress(@NonNull FocusedAreaProgress progress)
  {
    java.util.List<FocusedAreaProgressCallback> snapshot;
    synchronized (sProgressCallbacks)
    {
      snapshot = new java.util.ArrayList<>(sProgressCallbacks);
    }
    for (FocusedAreaProgressCallback cb : snapshot)
      cb.onFocusedAreaProgressChanged(progress);
  }

  static void notifyExplorationAreaTapped(@NonNull FocusedAreaProgress progress)
  {
    java.util.List<ExplorationAreaTapCallback> snapshot;
    synchronized (sTapCallbacks)
    {
      snapshot = new java.util.ArrayList<>(sTapCallbacks);
    }
    for (ExplorationAreaTapCallback cb : snapshot)
      cb.onExplorationAreaTapped(progress);
  }

  static void notifyFirstGoalProgress(@NonNull FirstGoalProgress progress)
  {
    java.util.List<FirstGoalProgressCallback> snapshot;
    synchronized (sFirstGoalCallbacks)
    {
      snapshot = new java.util.ArrayList<>(sFirstGoalCallbacks);
    }
    for (FirstGoalProgressCallback cb : snapshot)
      cb.onFirstGoalProgressChanged(progress);
  }

  static void notifyAreaMilestonePresentation(@Nullable AreaMilestonePresentation presentation)
  {
    java.util.List<AreaMilestonePresentationCallback> snapshot;
    synchronized (sAreaMilestoneCallbacks)
    {
      snapshot = new java.util.ArrayList<>(sAreaMilestoneCallbacks);
    }
    for (AreaMilestonePresentationCallback cb : snapshot)
      cb.onAreaMilestonePresentationChanged(presentation);
  }
}