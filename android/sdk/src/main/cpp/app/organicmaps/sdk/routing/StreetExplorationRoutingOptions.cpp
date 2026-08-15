#include <jni.h>
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "routing/routing_options.hpp"

namespace
{
routing::StreetExplorationRoutingMode ModeFromJni(jint mode)
{
  switch (static_cast<int>(mode))
  {
  case 1: return routing::StreetExplorationRoutingMode::Prefer;
  case 2: return routing::StreetExplorationRoutingMode::Avoid;
  default: return routing::StreetExplorationRoutingMode::Neither;
  }
}
}  // namespace

extern "C"
{
JNIEXPORT jint JNICALL
Java_app_organicmaps_sdk_routing_StreetExplorationRoutingOptions_nativeGetMode(JNIEnv *, jclass)
{
  routing::StreetExplorationRoutingOptions const options =
      routing::StreetExplorationRoutingOptions::LoadFromSettings();
  return static_cast<jint>(options.m_mode);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_routing_StreetExplorationRoutingOptions_nativeSetMode(JNIEnv *, jclass, jint mode)
{
  routing::StreetExplorationRoutingOptions options = routing::StreetExplorationRoutingOptions::LoadFromSettings();
  options.m_mode = ModeFromJni(mode);
  routing::StreetExplorationRoutingOptions::SaveToSettings(options);
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_sdk_routing_StreetExplorationRoutingOptions_nativeGetEnabled(JNIEnv *, jclass)
{
  routing::StreetExplorationRoutingOptions const options =
      routing::StreetExplorationRoutingOptions::LoadFromSettings();
  return static_cast<jboolean>(options.IsPreferEnabled());
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_routing_StreetExplorationRoutingOptions_nativeSetEnabled(JNIEnv *, jclass, jboolean enabled)
{
  routing::StreetExplorationRoutingOptions options = routing::StreetExplorationRoutingOptions::LoadFromSettings();
  options.m_mode = static_cast<bool>(enabled) ? routing::StreetExplorationRoutingMode::Prefer
                                              : routing::StreetExplorationRoutingMode::Neither;
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
