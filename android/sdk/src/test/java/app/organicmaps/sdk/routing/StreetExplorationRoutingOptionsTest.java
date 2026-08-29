package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class StreetExplorationRoutingOptionsTest
{
  @Test
  public void preferFallback_setsModePreferKeepsStrength_fromAvoid0()
  {
    StreetExplorationRoutingOptions current =
        new StreetExplorationRoutingOptions(StreetExplorationRoutingOptions.MODE_AVOID, 0);
    StreetExplorationRoutingOptions next = StreetExplorationRoutingOptions.preferFallback(current);
    assertEquals(StreetExplorationRoutingOptions.MODE_PREFER, next.m_mode);
    assertEquals(0.0, next.m_strength, 0.0);
  }

  @Test
  public void preferFallback_setsModePreferKeepsStrength_fromAvoid50()
  {
    StreetExplorationRoutingOptions current =
        new StreetExplorationRoutingOptions(StreetExplorationRoutingOptions.MODE_AVOID, 50);
    StreetExplorationRoutingOptions next = StreetExplorationRoutingOptions.preferFallback(current);
    assertEquals(StreetExplorationRoutingOptions.MODE_PREFER, next.m_mode);
    assertEquals(50.0, next.m_strength, 0.0);
  }

  @Test
  public void preferFallback_setsModePreferKeepsStrength_fromAvoid100()
  {
    StreetExplorationRoutingOptions current =
        new StreetExplorationRoutingOptions(StreetExplorationRoutingOptions.MODE_AVOID, 100);
    StreetExplorationRoutingOptions next = StreetExplorationRoutingOptions.preferFallback(current);
    assertEquals(StreetExplorationRoutingOptions.MODE_PREFER, next.m_mode);
    assertEquals(100.0, next.m_strength, 0.0);
  }

  @Test
  public void preferFallback_setsModePreferKeepsStrength_fromPrefer40()
  {
    StreetExplorationRoutingOptions current =
        new StreetExplorationRoutingOptions(StreetExplorationRoutingOptions.MODE_PREFER, 40);
    StreetExplorationRoutingOptions next = StreetExplorationRoutingOptions.preferFallback(current);
    assertEquals(StreetExplorationRoutingOptions.MODE_PREFER, next.m_mode);
    assertEquals(40.0, next.m_strength, 0.0);
  }

  @Test
  public void preferFallback_setsModePreferKeepsStrength_fromNeither25()
  {
    StreetExplorationRoutingOptions current =
        new StreetExplorationRoutingOptions(StreetExplorationRoutingOptions.MODE_NEITHER, 25);
    StreetExplorationRoutingOptions next = StreetExplorationRoutingOptions.preferFallback(current);
    assertEquals(StreetExplorationRoutingOptions.MODE_PREFER, next.m_mode);
    assertEquals(25.0, next.m_strength, 0.0);
  }

  @Test
  public void normalFallback_setsModeNeitherKeepsStrength_fromAvoid50()
  {
    StreetExplorationRoutingOptions current =
        new StreetExplorationRoutingOptions(StreetExplorationRoutingOptions.MODE_AVOID, 50);
    StreetExplorationRoutingOptions next = StreetExplorationRoutingOptions.normalFallback(current);
    assertEquals(StreetExplorationRoutingOptions.MODE_NEITHER, next.m_mode);
    assertEquals(50.0, next.m_strength, 0.0);
  }
}
