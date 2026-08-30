package app.organicmaps.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class ExploreDeepLinkTest
{
  @Test
  public void isAddFriend_matchesDedicatedHostAndHttpsPaths()
  {
    assertTrue(ExploreDeepLink.isAddFriend("add-friend", null));
    assertTrue(ExploreDeepLink.isAddFriend("ADD-FRIEND", ""));
    assertTrue(ExploreDeepLink.isAddFriend("streifzug.app", "/add-friend"));
    assertTrue(ExploreDeepLink.isAddFriend("comaps.at", "/add-friend"));
    assertTrue(ExploreDeepLink.isAddFriend("streifzug.app", "/Add-Friend/next"));
    assertTrue(ExploreDeepLink.isAddFriend("COMAPS.AT", "/add-friend/"));
  }

  @Test
  public void isAddFriend_ignoresGe0MapLinks()
  {
    assertFalse(ExploreDeepLink.isAddFriend("o4B4pYZsRs", "/Zoo"));
    assertFalse(ExploreDeepLink.isAddFriend("streifzug.app", "/o4B4pYZsRs"));
    assertFalse(ExploreDeepLink.isAddFriend("ge0.me", "/add-friend"));
    assertFalse(ExploreDeepLink.isAddFriend("map", null));
    assertFalse(ExploreDeepLink.isAddFriend(null, "/add-friend"));
  }

  @Test
  public void shouldPresentAddFriendOnboarding_hiddenInPublicV1()
  {
    assertFalse(FriendSettingsVisibility.friendsCapabilityEnabled());
    assertFalse(ExploreDeepLink.shouldPresentAddFriendOnboarding(null));
  }
}
