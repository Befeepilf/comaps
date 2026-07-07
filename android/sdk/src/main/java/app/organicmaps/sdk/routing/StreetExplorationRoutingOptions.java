package app.organicmaps.sdk.routing;

public class StreetExplorationRoutingOptions
{
  public boolean m_enabled;
  public double m_strength;

  public StreetExplorationRoutingOptions(boolean enabled, double strength)
  {
    this.m_enabled = enabled;
    this.m_strength = strength;
  }

  public static StreetExplorationRoutingOptions LoadFromSettings()
  {
    boolean enabled = nativeGetEnabled();
    double strength = nativeGetStrength();
    return new StreetExplorationRoutingOptions(enabled, strength);
  }

  public static void SaveToSettings(StreetExplorationRoutingOptions settings)
  {
    nativeSetEnabled(settings.m_enabled);
    nativeSetStrength(settings.m_strength);
  }

  private static native boolean nativeGetEnabled();
  private static native void nativeSetEnabled(boolean enabled);
  private static native double nativeGetStrength();
  private static native void nativeSetStrength(double strength);
}
