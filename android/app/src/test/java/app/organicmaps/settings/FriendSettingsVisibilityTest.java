package app.organicmaps.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class FriendSettingsVisibilityTest
{
  @Test
  public void friendsCapabilityEnabled_offInPublicV1()
  {
    assertFalse(FriendSettingsVisibility.friendsCapabilityEnabled());
  }

  @Test
  public void showFriendRows_hiddenWhenCapabilityOff()
  {
    assertFalse(FriendSettingsVisibility.showFriendRows(false));
    assertFalse(FriendSettingsVisibility.showFriendRows(FriendSettingsVisibility.friendsCapabilityEnabled()));
  }

  @Test
  public void showFriendRows_shownWhenCapabilityOn()
  {
    assertTrue(FriendSettingsVisibility.showFriendRows(true));
  }

  @Test
  public void showFriendFacingNicknameCopy_hiddenWhenCapabilityOff()
  {
    assertFalse(FriendSettingsVisibility.showFriendFacingNicknameCopy(false));
    assertFalse(FriendSettingsVisibility.showFriendFacingNicknameCopy(
        FriendSettingsVisibility.friendsCapabilityEnabled()));
  }

  @Test
  public void showFriendFacingNicknameCopy_shownWhenCapabilityOn()
  {
    assertTrue(FriendSettingsVisibility.showFriendFacingNicknameCopy(true));
  }

  @Test
  public void showAddFriendOnboarding_hiddenWhenCapabilityOff()
  {
    assertFalse(FriendSettingsVisibility.showAddFriendOnboarding(false));
    assertFalse(FriendSettingsVisibility.showAddFriendOnboarding(
        FriendSettingsVisibility.friendsCapabilityEnabled()));
  }

  @Test
  public void showAddFriendOnboarding_shownWhenCapabilityOn()
  {
    assertTrue(FriendSettingsVisibility.showAddFriendOnboarding(true));
  }
}
