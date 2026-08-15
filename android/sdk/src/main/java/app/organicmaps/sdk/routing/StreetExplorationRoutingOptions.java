package app.organicmaps.sdk.routing;

public class StreetExplorationRoutingOptions
{
  public static final int MODE_NEITHER = 0;
  public static final int MODE_PREFER = 1;
  public static final int MODE_AVOID = 2;

  public int m_mode;
  public double m_strength;

  public StreetExplorationRoutingOptions(int mode, double strength)
  {
    this.m_mode = mode;
    this.m_strength = strength;
  }

  public boolean isPreferEnabled() { return m_mode == MODE_PREFER; }

  public boolean isAvoidEnabled() { return m_mode == MODE_AVOID; }

  public static StreetExplorationRoutingOptions preferFallback(StreetExplorationRoutingOptions current)
  {
    return new StreetExplorationRoutingOptions(MODE_PREFER, current.m_strength);
  }

  public static StreetExplorationRoutingOptions LoadFromSettings()
  {
    int mode = nativeGetMode();
    double strength = nativeGetStrength();
    return new StreetExplorationRoutingOptions(mode, strength);
  }

  public static void SaveToSettings(StreetExplorationRoutingOptions settings)
  {
    nativeSetMode(settings.m_mode);
    nativeSetStrength(settings.m_strength);
  }

  private static native int nativeGetMode();
  private static native void nativeSetMode(int mode);
  private static native boolean nativeGetEnabled();
  private static native void nativeSetEnabled(boolean enabled);
  private static native double nativeGetStrength();
  private static native void nativeSetStrength(double strength);
}
