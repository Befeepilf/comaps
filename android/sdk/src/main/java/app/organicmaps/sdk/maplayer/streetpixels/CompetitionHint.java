package app.organicmaps.sdk.maplayer.streetpixels;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

@Keep
public class CompetitionHint
{
  public static final int KIND_AHEAD = 0;
  public static final int KIND_APPROACHING = 1;
  public static final int KIND_LEADS = 2;
  public static final int KIND_COMPARE_AREA = 3;
  public static final int KIND_COMPARE_GENERIC = 4;

  public final int kind;
  @NonNull
  public final String areaName;
  @NonNull
  public final String text;

  @Keep
  public CompetitionHint(int kind, @NonNull String areaName, @NonNull String text)
  {
    this.kind = kind;
    this.areaName = areaName;
    this.text = text;
  }
}
