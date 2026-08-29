package app.organicmaps.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class FriendSettingsVisibilityTest
{
  @Test
  public void showFriendRows_hiddenWhenCapabilityOff()
  {
    assertFalse(FriendSettingsVisibility.showFriendRows(false));
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
  }

  @Test
  public void showFriendFacingNicknameCopy_shownWhenCapabilityOn()
  {
    assertTrue(FriendSettingsVisibility.showFriendFacingNicknameCopy(true));
  }
}
