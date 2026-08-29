package app.organicmaps.settings;

import static org.junit.Assert.assertFalse;

import org.junit.Test;

public class PublicSettingsVisibilityTest
{
  @Test
  public void publicBuild_hidesGpxAndFriends()
  {
    assertFalse(GpxSettingsVisibility.showGpxScreen(false, false, false, false));
    assertFalse(GpxSettingsVisibility.showImportRow(false));
    assertFalse(GpxSettingsVisibility.showExportRow(false));
    assertFalse(GpxSettingsVisibility.showBatchImportRow(false, false));
    assertFalse(GpxSettingsVisibility.showInfoPage(false, false, false));
    assertFalse(FriendSettingsVisibility.showFriendRows(FriendSettingsVisibility.friendsCapabilityEnabled()));
    assertFalse(FriendSettingsVisibility.showFriendFacingNicknameCopy(
        FriendSettingsVisibility.friendsCapabilityEnabled()));
  }
}
