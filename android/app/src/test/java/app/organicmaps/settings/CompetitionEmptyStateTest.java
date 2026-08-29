package app.organicmaps.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class CompetitionEmptyStateTest
{
  @Test
  public void showRankingRows_hiddenWhenEmpty()
  {
    assertFalse(CompetitionEmptyState.showRankingRows(0));
    assertFalse(CompetitionEmptyState.showRankingRows(-1));
  }

  @Test
  public void showRankingRows_shownWhenAnyCompetitor()
  {
    assertTrue(CompetitionEmptyState.showRankingRows(1));
    assertTrue(CompetitionEmptyState.showRankingRows(4));
  }
}
