#include <jni.h>

#include "map/explorer_pro_analytics.hpp"

extern "C"
{
JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_ExplorerProAnalytics_nativeRecordInfoPageViewed(JNIEnv *, jclass)
{
  street_pixels::ExplorerProAnalytics::RecordInfoPageViewed();
}

JNIEXPORT jlong JNICALL
Java_app_organicmaps_sdk_ExplorerProAnalytics_nativeGetInfoPageViewed(JNIEnv *, jclass)
{
  return static_cast<jlong>(street_pixels::ExplorerProAnalytics::LoadSnapshot().m_infoPageViewed);
}

JNIEXPORT jlong JNICALL
Java_app_organicmaps_sdk_ExplorerProAnalytics_nativeGetGpxImportUsage(JNIEnv *, jclass)
{
  return static_cast<jlong>(street_pixels::ExplorerProAnalytics::LoadSnapshot().m_gpxImportUsage);
}

JNIEXPORT jlong JNICALL
Java_app_organicmaps_sdk_ExplorerProAnalytics_nativeGetGpxExportUsage(JNIEnv *, jclass)
{
  return static_cast<jlong>(street_pixels::ExplorerProAnalytics::LoadSnapshot().m_gpxExportUsage);
}
}
