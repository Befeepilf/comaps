package app.organicmaps.sdk;

import org.junit.After;
import org.junit.Test;

public class ExplorerProAnalyticsTest
{
  @After
  public void resetNativeReady()
  {
    ExplorerPro.setNativeReady(false);
  }

  @Test
  public void recordInfoPageViewed_doesNotThrowWhenNativeNotReady()
  {
    ExplorerPro.setNativeReady(false);
    ExplorerProAnalytics.recordInfoPageViewed();
  }
}
