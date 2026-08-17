#include <jni.h>

#include "routing/street_exploration_routing_analytics.hpp"

extern "C"
{
JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_routing_StreetExplorationRoutingAnalytics_nativeRecordAvoidFallbackPrefer(JNIEnv *, jclass)
{
  routing::StreetExplorationRoutingAnalytics::RecordAvoidFallbackPrefer();
}

JNIEXPORT jlong JNICALL
Java_app_organicmaps_sdk_routing_StreetExplorationRoutingAnalytics_nativeGetPreferUsed(JNIEnv *, jclass)
{
  return static_cast<jlong>(routing::StreetExplorationRoutingAnalytics::LoadSnapshot().m_preferUsed);
}

JNIEXPORT jlong JNICALL
Java_app_organicmaps_sdk_routing_StreetExplorationRoutingAnalytics_nativeGetAvoidUsed(JNIEnv *, jclass)
{
  return static_cast<jlong>(routing::StreetExplorationRoutingAnalytics::LoadSnapshot().m_avoidUsed);
}

JNIEXPORT jlong JNICALL
Java_app_organicmaps_sdk_routing_StreetExplorationRoutingAnalytics_nativeGetAvoidFallbackPrefer(JNIEnv *, jclass)
{
  return static_cast<jlong>(routing::StreetExplorationRoutingAnalytics::LoadSnapshot().m_avoidFallbackPrefer);
}
}
