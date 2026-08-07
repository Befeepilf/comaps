package app.organicmaps.sdk.maplayer.streetpixels;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

@Keep
public class RematchFractionChange
{
  @NonNull
  public final String countryId;
  public final long previousTotal;
  public final long previousExplored;
  public final long newTotal;
  public final long newExplored;
  public final double previousFraction;
  public final double newFraction;
  public final boolean decreasedDueToUniverseGrowth;

  @Keep
  public RematchFractionChange(@NonNull String countryId, long previousTotal, long previousExplored, long newTotal,
                               long newExplored, double previousFraction, double newFraction,
                               boolean decreasedDueToUniverseGrowth)
  {
    this.countryId = countryId;
    this.previousTotal = previousTotal;
    this.previousExplored = previousExplored;
    this.newTotal = newTotal;
    this.newExplored = newExplored;
    this.previousFraction = previousFraction;
    this.newFraction = newFraction;
    this.decreasedDueToUniverseGrowth = decreasedDueToUniverseGrowth;
  }
}
