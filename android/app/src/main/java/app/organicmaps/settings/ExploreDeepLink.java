package app.organicmaps.settings;

import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public final class ExploreDeepLink
{
  private ExploreDeepLink() {}

  public static boolean isAddFriendUri(@Nullable Uri uri)
  {
    if (uri == null)
      return false;

    if ("add-friend".equals(uri.getHost()))
      return true;

    String host = uri.getHost();
    if (host == null)
      return false;

    if (("comaps.app".equals(host) || "comaps.at".equals(host)) && uri.getPath() != null
        && uri.getPath().startsWith("/add-friend"))
      return true;

    return false;
  }

  @Nullable
  public static String getAddFriendUsername(@NonNull Uri uri)
  {
    String username = uri.getQueryParameter("u");
    if (username == null || username.isEmpty())
      return null;
    return username.trim().toLowerCase();
  }
}
