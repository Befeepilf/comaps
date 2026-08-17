package app.organicmaps.sdk.routing;

public final class StreetExplorationRoutingAnalytics
{
  private StreetExplorationRoutingAnalytics() {}

  public static void recordAvoidFallbackPrefer()
  {
    nativeRecordAvoidFallbackPrefer();
  }

  public static long getPreferUsed()
  {
    return nativeGetPreferUsed();
  }

  public static long getAvoidUsed()
  {
    return nativeGetAvoidUsed();
  }

  public static long getAvoidFallbackPrefer()
  {
    return nativeGetAvoidFallbackPrefer();
  }

  private static native void nativeRecordAvoidFallbackPrefer();
  private static native long nativeGetPreferUsed();
  private static native long nativeGetAvoidUsed();
  private static native long nativeGetAvoidFallbackPrefer();
}
