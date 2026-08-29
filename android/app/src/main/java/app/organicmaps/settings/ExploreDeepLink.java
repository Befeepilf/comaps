package app.organicmaps.settings;

import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Locale;

public final class ExploreDeepLink
{
  private ExploreDeepLink() {}

  public static boolean isAddFriendUri(@Nullable Uri uri)
  {
    if (uri == null)
      return false;
    return isAddFriend(uri.getHost(), uri.getPath());
  }

  /**
   * Public V1 must not present add-friend onboarding even when a generic
   * {@code comaps://} or {@code https://comaps.at} VIEW filter delivered the URI.
   */
  public static boolean shouldPresentAddFriendOnboarding(@Nullable Uri uri)
  {
    return FriendSettingsVisibility.showAddFriendOnboarding(
               FriendSettingsVisibility.friendsCapabilityEnabled())
        && isAddFriendUri(uri);
  }

  static boolean isAddFriend(@Nullable String host, @Nullable String path)
  {
    if (host != null && "add-friend".equalsIgnoreCase(host))
      return true;
    if (host == null || path == null)
      return false;
    if (!("comaps.app".equalsIgnoreCase(host) || "comaps.at".equalsIgnoreCase(host)))
      return false;
    String lowerPath = path.toLowerCase(Locale.ROOT);
    return "/add-friend".equals(lowerPath) || lowerPath.startsWith("/add-friend/");
  }

  @Nullable
  public static String getAddFriendUsername(@NonNull Uri uri)
  {
    String username = uri.getQueryParameter("u");
    if (username == null || username.isEmpty())
      return null;
    return username.trim().toLowerCase(Locale.ROOT);
  }
}
