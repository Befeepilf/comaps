package app.organicmaps.sdk.maplayer.streetpixels;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

@Keep
public class CompletionCardModel
{
  @NonNull
  public final String areaDisplayName;
  @NonNull
  public final String headline;
  @NonNull
  public final float[] outlineXs;
  @NonNull
  public final float[] outlineYs;
  @NonNull
  public final int[] ringLengths;
  @Nullable
  public final String nickname;
  @Nullable
  public final String completedDate;
  @NonNull
  public final String branding;
  @NonNull
  public final String competitionLine;

  @Keep
  public CompletionCardModel(@NonNull String areaDisplayName, @NonNull String headline, @NonNull float[] outlineXs,
                             @NonNull float[] outlineYs, @NonNull int[] ringLengths, @Nullable String nickname,
                             @Nullable String completedDate, @NonNull String branding,
                             @NonNull String competitionLine)
  {
    this.areaDisplayName = areaDisplayName;
    this.headline = headline;
    this.outlineXs = outlineXs;
    this.outlineYs = outlineYs;
    this.ringLengths = ringLengths;
    this.nickname = nickname;
    this.completedDate = completedDate;
    this.branding = branding;
    this.competitionLine = competitionLine;
  }
}
