package app.organicmaps.sdk.maplayer.streetpixels;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

@Keep
public class CompetitionRankingRow
{
  public final int rank;
  @NonNull
  public final String displayName;
  public final double decayedScore;
  public final boolean isCurrentUser;

  @Keep
  public CompetitionRankingRow(int rank, @Nullable String displayName, double decayedScore, boolean isCurrentUser)
  {
    this.rank = rank;
    this.displayName = displayName == null ? "" : displayName;
    this.decayedScore = decayedScore;
    this.isCurrentUser = isCurrentUser;
  }
}
