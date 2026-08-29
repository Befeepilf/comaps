package app.organicmaps.settings;

public final class FirstRunFlow
{
  private FirstRunFlow() {}

  public static boolean requestLocationOnAppOpen()
  {
    return false;
  }

  public static boolean shouldShowExploringCard(boolean cardSeen)
  {
    return !cardSeen;
  }

  public static boolean bundleCompetitionWithLocationRationale()
  {
    return false;
  }
}
