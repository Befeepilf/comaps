package app.organicmaps.sdk.maplayer.streetpixels;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

@Keep
public class CompetitionAreaChrome
{
  public final boolean offline;
  public final boolean stale;
  public final boolean unclaimed;
  public final boolean contested;
  @NonNull
  public final String bossLine;
  @NonNull
  public final String gapLine;
  @NonNull
  public final CompetitionRankingRow[] rankingRows;
  public final double localOwnershipScore;
  public final double personalCompletionFraction;
  public final boolean localEligible;
  public final boolean localIsBoss;

  @Keep
  public CompetitionAreaChrome(boolean offline, boolean stale, boolean unclaimed, boolean contested,
                               @Nullable String bossLine, @Nullable String gapLine,
                               @Nullable CompetitionRankingRow[] rankingRows, double localOwnershipScore,
                               double personalCompletionFraction, boolean localEligible, boolean localIsBoss)
  {
    this.offline = offline;
    this.stale = stale;
    this.unclaimed = unclaimed;
    this.contested = contested;
    this.bossLine = bossLine == null ? "" : bossLine;
    this.gapLine = gapLine == null ? "" : gapLine;
    this.rankingRows = rankingRows == null ? new CompetitionRankingRow[0] : rankingRows;
    this.localOwnershipScore = localOwnershipScore;
    this.personalCompletionFraction = personalCompletionFraction;
    this.localEligible = localEligible;
    this.localIsBoss = localIsBoss;
  }
}
