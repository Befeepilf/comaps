package app.organicmaps.sdk.maplayer.streetpixels;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

@Keep
public class AreaMilestonePresentation
{
  public static final int THRESHOLD_25 = 0;
  public static final int THRESHOLD_50 = 1;
  public static final int THRESHOLD_100 = 2;

  public final int threshold;
  public final long osmId;
  public final int compactIndex;
  @NonNull
  public final String displayName;
  @NonNull
  public final String competitionLine;
  public final boolean debugPreview;

  @Keep
  public AreaMilestonePresentation(int threshold, long osmId, int compactIndex, @NonNull String displayName,
                                   @NonNull String competitionLine, boolean debugPreview)
  {
    this.threshold = threshold;
    this.osmId = osmId;
    this.compactIndex = compactIndex;
    this.displayName = displayName;
    this.competitionLine = competitionLine;
    this.debugPreview = debugPreview;
  }
}
