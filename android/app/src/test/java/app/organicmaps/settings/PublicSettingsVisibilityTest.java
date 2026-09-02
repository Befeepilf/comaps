package app.organicmaps.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class PublicSettingsVisibilityTest
{
  @Test
  public void publicBuild_hidesProGpxAndFriends_showsExport()
  {
    assertFalse(GpxSettingsVisibility.showImportRow(false));
    assertTrue(GpxSettingsVisibility.showExportRow(true));
    assertFalse(GpxSettingsVisibility.showBatchImportRow(false, false));
    assertFalse(GpxSettingsVisibility.showInfoPage(false, false, false));
    assertTrue(GpxSettingsVisibility.showGpxScreen(false, true, false, false));
    assertFalse(FriendSettingsVisibility.showFriendRows(FriendSettingsVisibility.friendsCapabilityEnabled()));
    assertFalse(FriendSettingsVisibility.showFriendFacingNicknameCopy(
        FriendSettingsVisibility.friendsCapabilityEnabled()));
    assertFalse(FriendSettingsVisibility.showAddFriendOnboarding(
        FriendSettingsVisibility.friendsCapabilityEnabled()));
  }
}
