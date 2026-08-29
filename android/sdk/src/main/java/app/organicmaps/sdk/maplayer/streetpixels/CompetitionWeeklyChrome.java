package app.organicmaps.sdk.maplayer.streetpixels;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

@Keep
public class CompetitionWeeklyChrome
{
  public final boolean offline;
  @NonNull
  public final String body;
  @NonNull
  public final String[] rows;

  @Keep
  public CompetitionWeeklyChrome(boolean offline, @Nullable String body, @Nullable String[] rows)
  {
    this.offline = offline;
    this.body = body == null ? "" : body;
    this.rows = rows == null ? new String[0] : rows;
  }
}
