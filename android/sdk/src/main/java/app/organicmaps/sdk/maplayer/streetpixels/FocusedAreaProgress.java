package app.organicmaps.sdk.maplayer.streetpixels;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import java.util.Locale;

@Keep
public class FocusedAreaProgress
{
  public final boolean hasFocus;
  public final boolean fractionValid;
  public final boolean citySummary;
  public final boolean areaCompleted;
  public final boolean noExplorationArea;
  public final boolean previouslyCompleted;
  public final int compactIndex;
  public final long osmId;
  @NonNull
  public final String displayName;
  public final double fraction;

  @Keep
  public FocusedAreaProgress(boolean hasFocus, boolean fractionValid, boolean citySummary, boolean areaCompleted,
                             boolean noExplorationArea, int compactIndex, long osmId, @NonNull String displayName,
                             double fraction, boolean previouslyCompleted)
  {
    this.hasFocus = hasFocus;
    this.fractionValid = fractionValid;
    this.citySummary = citySummary;
    this.areaCompleted = areaCompleted;
    this.noExplorationArea = noExplorationArea;
    this.compactIndex = compactIndex;
    this.osmId = osmId;
    this.displayName = displayName;
    this.fraction = fraction;
    this.previouslyCompleted = previouslyCompleted;
  }

  @NonNull
  public static String formatPercent(double fraction)
  {
    if (!(fraction > 0.0))
      return "0%";
    double percent = Math.round(fraction * 1000.0) / 10.0;
    if (percent < 0.1)
      return "<0.1%";
    if (percent == Math.rint(percent))
      return String.format(Locale.US, "%.0f%%", percent);
    return String.format(Locale.US, "%.1f%%", percent);
  }
}
