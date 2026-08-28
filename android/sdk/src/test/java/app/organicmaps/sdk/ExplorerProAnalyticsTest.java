package app.organicmaps.sdk;

import static org.junit.Assert.assertEquals;

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

  @Test
  public void getters_returnZeroWhenNativeNotReady()
  {
    ExplorerPro.setNativeReady(false);
    assertEquals(0L, ExplorerProAnalytics.getInfoPageViewed());
    assertEquals(0L, ExplorerProAnalytics.getGpxImportUsage());
    assertEquals(0L, ExplorerProAnalytics.getGpxExportUsage());
  }
}
