package app.organicmaps.settings;

public final class FriendSettingsVisibility
{
  private FriendSettingsVisibility() {}

  public static boolean friendsCapabilityEnabled()
  {
    return false;
  }

  public static boolean showFriendRows(boolean friendsCapabilityEnabled)
  {
    return friendsCapabilityEnabled;
  }

  public static boolean showFriendFacingNicknameCopy(boolean friendsCapabilityEnabled)
  {
    return friendsCapabilityEnabled;
  }
}
