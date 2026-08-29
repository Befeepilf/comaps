package app.organicmaps.settings;

public final class IncompleteSpaSettingsVisibility
{
  private IncompleteSpaSettingsVisibility() {}

  public static boolean showRow(int incompleteCount)
  {
    return incompleteCount > 0;
  }
}
