package app.organicmaps.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class RoutePlanningUiTest
{
  @Test
  public void layersButton_visibleWhilePlanningNotNavigating()
  {
    assertTrue(RoutePlanningUi.shouldShowLayersButton(true, false));
  }

  @Test
  public void layersButton_hiddenDuringNavigation()
  {
    assertFalse(RoutePlanningUi.shouldShowLayersButton(true, true));
  }

  @Test
  public void layersButton_respectsRequestedHidden()
  {
    assertFalse(RoutePlanningUi.shouldShowLayersButton(false, false));
  }

  @Test
  public void layersTopMargin_usesHeaderPlusPadding()
  {
    assertEquals(136, RoutePlanningUi.layersTopMarginPx(128, 8));
  }

  @Test
  public void layersTopMargin_unmeasuredHeaderKeepsXmlDefault()
  {
    assertEquals(-1, RoutePlanningUi.layersTopMarginPx(0, 8));
    assertEquals(-1, RoutePlanningUi.layersTopMarginPx(-20, 8));
  }

  @Test
  public void startRecording_defaultOffDoesNotStart()
  {
    assertFalse(RoutePlanningUi.shouldStartRecordingOnRouteStart(false, false));
  }

  @Test
  public void startRecording_toggleOnStartsWhenIdle()
  {
    assertTrue(RoutePlanningUi.shouldStartRecordingOnRouteStart(true, false));
  }

  @Test
  public void startRecording_toggleOnDoesNotRestartActiveSession()
  {
    assertFalse(RoutePlanningUi.shouldStartRecordingOnRouteStart(true, true));
  }
}
