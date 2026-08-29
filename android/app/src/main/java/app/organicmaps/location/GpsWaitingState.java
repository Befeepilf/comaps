package app.organicmaps.location;

public final class GpsWaitingState
{
  public static final float ACCURACY_LIMIT_METERS = 25.0f;

  private GpsWaitingState() {}

  public static boolean showWaiting(boolean recordingActive, boolean paused, boolean hasLocation,
                                    boolean hasAccuracy, float accuracyMeters)
  {
    if (!recordingActive || paused)
      return false;
    if (!hasLocation || !hasAccuracy)
      return true;
    return accuracyMeters > ACCURACY_LIMIT_METERS;
  }
}
