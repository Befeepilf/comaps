package app.organicmaps.maplayer;

import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import app.organicmaps.sdk.maplayer.streetpixels.CompletionCardSharePayload;
import app.organicmaps.sdk.util.StorageUtils;

public final class CompletionCardShare
{
  private CompletionCardShare() {}

  public static void shareImage(@NonNull Context context, @NonNull CompletionCardSharePayload payload)
  {
    Uri uri = StorageUtils.getUriForFilePath(context, payload.path);
    Intent intent = new Intent(Intent.ACTION_SEND);
    intent.setType(payload.mimeType);
    intent.putExtra(Intent.EXTRA_STREAM, uri);
    if (!TextUtils.isEmpty(payload.text))
      intent.putExtra(Intent.EXTRA_TEXT, payload.text);
    intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
    if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.LOLLIPOP_MR1)
    {
      intent.setClipData(ClipData.newRawUri("", uri));
      intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
    }
    context.startActivity(Intent.createChooser(intent, context.getString(R.string.share)));
  }
}
