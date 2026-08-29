package app.organicmaps.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class CompetitionSettingsTest
{
  @Test
  public void showAccountSyncSwitch_never()
  {
    assertFalse(CompetitionSettings.showAccountSyncSwitch());
  }

  @Test
  public void writeEnabledFromAccountSave_never()
  {
    assertFalse(CompetitionSettings.writeEnabledFromAccountSave());
  }
}
