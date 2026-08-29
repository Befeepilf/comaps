#include <jni.h>

#include "map/product_analytics.hpp"

extern "C"
{
JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_ProductAnalytics_nativeRecordPositionPermissionGranted(JNIEnv *, jclass)
{
  street_pixels::ProductAnalytics::RecordPositionPermissionGranted();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_ProductAnalytics_nativeRecordNotifyPermissionGranted(JNIEnv *, jclass)
{
  street_pixels::ProductAnalytics::RecordNotifyPermissionGranted();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_ProductAnalytics_nativeRecordCompetitionPromptViewed(JNIEnv *, jclass)
{
  street_pixels::ProductAnalytics::RecordCompetitionPromptViewed();
}
}
