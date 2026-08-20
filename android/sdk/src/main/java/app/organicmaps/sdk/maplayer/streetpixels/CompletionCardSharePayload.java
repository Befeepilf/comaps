package app.organicmaps.sdk.maplayer.streetpixels;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

@Keep
public class CompletionCardSharePayload
{
  @NonNull
  public final String path;
  @NonNull
  public final String mimeType;
  @NonNull
  public final String text;

  @Keep
  public CompletionCardSharePayload(@NonNull String path, @NonNull String mimeType, @NonNull String text)
  {
    this.path = path;
    this.mimeType = mimeType;
    this.text = text;
  }
}
