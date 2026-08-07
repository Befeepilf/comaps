package app.organicmaps.sdk.maplayer.streetpixels;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

@Keep
public class FocusedAreaProgress
{
  public final boolean hasFocus;
  public final boolean fractionValid;
  public final int compactIndex;
  public final long osmId;
  @NonNull
  public final String displayName;
  public final double fraction;

  @Keep
  public FocusedAreaProgress(boolean hasFocus, boolean fractionValid, int compactIndex, long osmId,
                             @NonNull String displayName, double fraction)
  {
    this.hasFocus = hasFocus;
    this.fractionValid = fractionValid;
    this.compactIndex = compactIndex;
    this.osmId = osmId;
    this.displayName = displayName;
    this.fraction = fraction;
  }
}
