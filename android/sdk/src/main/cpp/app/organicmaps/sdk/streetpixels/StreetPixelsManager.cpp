#include <jni.h>
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/platform/AndroidPlatform.hpp"

#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/focused_area_progress.hpp"

#include "platform/platform.hpp"

#include "geometry/point2d.hpp"

extern "C"
{
static void StreetPixelsStateChanged(bool enabled, StreetPixelsManager::StreetPixelsStatus status,
                                     std::string countryId, std::shared_ptr<jobject> const & listener)
{
  JNIEnv * env = jni::GetEnv();
  env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener, "onStateChanged", "(ZILjava/lang/String;)V"),
                      static_cast<jboolean>(enabled), static_cast<jint>(status), jni::ToJavaString(env, countryId));
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeAddListener(
  JNIEnv * env, jclass clazz, jobject listener)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto globalListener = jni::make_global_ref(listener);
  g_framework->SetStreetPixelsListener(
    [globalListener](bool enabled, StreetPixelsManager::StreetPixelsStatus status, std::string const & countryId)
    {
        StreetPixelsStateChanged(enabled, status, countryId, globalListener);
    }
  );
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeRemoveListener(JNIEnv * env,
                                                                                                           jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  g_framework->SetStreetPixelsListener(nullptr);
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeShouldShowNotification(JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto const & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  auto const enabled = manager.GetState().enabled;
  return static_cast<jboolean>(enabled);
}

JNIEXPORT jdouble JNICALL Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeGetTotalExploredFraction(
  JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto const & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  double frac = manager.GetTotalExploredFraction();
  return static_cast<jdouble>(frac);
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeTakePendingRematchFractionChange(
    JNIEnv * env, jclass clazz, jstring countryId)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  auto const change = manager.TakePendingRematchFractionChange(jni::ToNativeString(env, countryId));
  if (!change)
    return nullptr;

  static jclass const changeClass = jni::GetGlobalClassRef(
      env, "app/organicmaps/sdk/maplayer/streetpixels/RematchFractionChange");
  static jmethodID const ctor =
      jni::GetConstructorID(env, changeClass, "(Ljava/lang/String;JJJJDDZ)V");
  jni::TScopedLocalRef const jCountryId(env, jni::ToJavaString(env, change->countryId));
  jobject const result =
      env->NewObject(changeClass, ctor, jCountryId.get(), static_cast<jlong>(change->previousTotal),
                     static_cast<jlong>(change->previousExplored), static_cast<jlong>(change->newTotal),
                     static_cast<jlong>(change->newExplored), static_cast<jdouble>(change->previousFraction),
                     static_cast<jdouble>(change->newFraction),
                     static_cast<jboolean>(change->decreasedDueToUniverseGrowth));
  return result;
}

static jobject ToJavaFocusedAreaProgress(JNIEnv * env, street_pixels::FocusedAreaProgress const & progress)
{
  static jclass const progressClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/maplayer/streetpixels/FocusedAreaProgress");
  static jmethodID const ctor = jni::GetConstructorID(env, progressClass, "(ZZIJLjava/lang/String;D)V");
  jni::TScopedLocalRef const jName(env, jni::ToJavaString(env, progress.m_displayName));
  return env->NewObject(progressClass, ctor, static_cast<jboolean>(progress.m_hasFocus),
                        static_cast<jboolean>(progress.m_fractionValid),
                        static_cast<jint>(progress.m_compactIndex), static_cast<jlong>(progress.m_osmId),
                        jName.get(), static_cast<jdouble>(progress.m_fraction));
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeGetFocusedAreaProgress(JNIEnv * env,
                                                                                              jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto const & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  return ToJavaFocusedAreaProgress(env, manager.GetFocusedAreaProgress());
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeRefreshFocusedAreaAtMapCenter(
    JNIEnv * env, jclass clazz, jstring countryId)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & native = *g_framework->NativeFramework();
  auto & manager = native.GetStreetPixelsManager();
  std::string const country = jni::ToNativeString(env, countryId);
  if (country.empty())
  {
    manager.ClearFocusedArea();
    return ToJavaFocusedAreaProgress(env, manager.GetFocusedAreaProgress());
  }

  std::string spaPath;
  auto localFile = g_framework->GetStorage().GetLatestLocalFile(country);
  if (localFile && localFile->OnDisk(MapFileType::Map))
    spaPath = street_pixels::ExplorationSidecarPathBesideMwm(localFile->GetPath(MapFileType::Map));
  else
    spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), country);

  int64_t const mapDataVersion = manager.GetPixMapDataVersion();
  if (!manager.IsAreaCompletionCacheValid())
    manager.RebuildAreaCompletionCache(country, spaPath, mapDataVersion);

  m2::PointD const centre = g_framework->GetViewportCenter();
  manager.TryFocusAtPoint(centre, spaPath, mapDataVersion);
  return ToJavaFocusedAreaProgress(env, manager.GetFocusedAreaProgress());
}
}
