package app.organicmaps.settings;

public final class CompetitionEmptyState
{
  private CompetitionEmptyState() {}

  public static boolean showRankingRows(int rankingCount)
  {
    return rankingCount > 0;
  }

  public static boolean showWeeklyBoard(int weeklyLineCount)
  {
    return weeklyLineCount > 0;
  }
}
