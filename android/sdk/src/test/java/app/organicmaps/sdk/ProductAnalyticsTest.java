package app.organicmaps.sdk;

import org.junit.After;
import org.junit.Test;

public class ProductAnalyticsTest
{
  @After
  public void resetNativeReady()
  {
    ExplorerPro.setNativeReady(false);
  }

  @Test
  public void recordMethods_doNotThrowWhenNativeNotReady()
  {
    ExplorerPro.setNativeReady(false);
    ProductAnalytics.recordPositionPermissionGranted();
    ProductAnalytics.recordNotifyPermissionGranted();
    ProductAnalytics.recordCompetitionPromptViewed();
  }
}
