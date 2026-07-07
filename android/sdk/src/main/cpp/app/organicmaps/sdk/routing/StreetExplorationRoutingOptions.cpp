#include <jni.h>
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "routing/routing_options.hpp"

extern "C"
{
JNIEXPORT jboolean JNICALL
Java_app_organicmaps_sdk_routing_StreetExplorationRoutingOptions_nativeGetEnabled(JNIEnv *, jclass)
{
  routing::StreetExplorationRoutingOptions const options =
      routing::StreetExplorationRoutingOptions::LoadFromSettings();
  return static_cast<jboolean>(options.m_enabled);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_routing_StreetExplorationRoutingOptions_nativeSetEnabled(JNIEnv *, jclass, jboolean enabled)
{
  routing::StreetExplorationRoutingOptions options = routing::StreetExplorationRoutingOptions::LoadFromSettings();
  options.m_enabled = static_cast<bool>(enabled);
  routing::StreetExplorationRoutingOptions::SaveToSettings(options);
}

JNIEXPORT jdouble JNICALL
Java_app_organicmaps_sdk_routing_StreetExplorationRoutingOptions_nativeGetStrength(JNIEnv *, jclass)
{
  routing::StreetExplorationRoutingOptions const options =
      routing::StreetExplorationRoutingOptions::LoadFromSettings();
  return static_cast<jdouble>(options.m_strength);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_routing_StreetExplorationRoutingOptions_nativeSetStrength(JNIEnv *, jclass, jdouble strength)
{
  routing::StreetExplorationRoutingOptions options = routing::StreetExplorationRoutingOptions::LoadFromSettings();
  options.m_strength = static_cast<double>(strength);
  routing::StreetExplorationRoutingOptions::SaveToSettings(options);
}
}
