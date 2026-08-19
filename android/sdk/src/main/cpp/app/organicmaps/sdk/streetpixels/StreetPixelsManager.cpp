#include <jni.h>
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/platform/AndroidPlatform.hpp"

#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/focus_selection_engine.hpp"
#include "street_pixels_areas/focused_area_progress.hpp"

#include "map/area_milestone_presentation.hpp"
#include "map/first_goal.hpp"

#include "platform/local_country_file.hpp"
#include "platform/location.hpp"
#include "platform/platform.hpp"

#include "storage/storage_defines.hpp"

#include "geometry/point2d.hpp"
#include "geometry/mercator.hpp"

#include <optional>

extern "C"
{
static jobject ToJavaFocusedAreaProgress(JNIEnv * env, street_pixels::FocusedAreaProgress const & progress)
{
  static jclass const progressClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/maplayer/streetpixels/FocusedAreaProgress");
  static jmethodID const ctor = jni::GetConstructorID(env, progressClass, "(ZZZZZIJLjava/lang/String;DZ)V");
  jni::TScopedLocalRef const jName(env, jni::ToJavaString(env, progress.m_displayName));
  return env->NewObject(progressClass, ctor, static_cast<jboolean>(progress.m_hasFocus),
                        static_cast<jboolean>(progress.m_fractionValid),
                        static_cast<jboolean>(progress.m_citySummary),
                        static_cast<jboolean>(progress.m_areaCompleted),
                        static_cast<jboolean>(progress.m_noExplorationArea),
                        static_cast<jint>(progress.m_compactIndex), static_cast<jlong>(progress.m_osmId),
                        jName.get(), static_cast<jdouble>(progress.m_fraction),
                        static_cast<jboolean>(progress.m_previouslyCompleted));
}

static void StreetPixelsStateChanged(bool enabled, StreetPixelsManager::StreetPixelsStatus status,
                                     std::string countryId, std::shared_ptr<jobject> const & listener)
{
  JNIEnv * env = jni::GetEnv();
  env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener, "onStateChanged", "(ZILjava/lang/String;)V"),
                      static_cast<jboolean>(enabled), static_cast<jint>(status), jni::ToJavaString(env, countryId));
}

static jobject ToJavaFirstGoalProgress(JNIEnv * env, street_pixels::FirstGoalProgress const & progress)
{
  static jclass const progressClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/maplayer/streetpixels/FirstGoalProgress");
  static jmethodID const ctor = jni::GetConstructorID(env, progressClass, "(III)V");
  return env->NewObject(progressClass, ctor, static_cast<jint>(progress.m_state),
                        static_cast<jint>(progress.m_collected), static_cast<jint>(progress.m_threshold));
}

static void CallFirstGoalCallback(std::shared_ptr<jobject> const & listener,
                                  street_pixels::FirstGoalProgress const & progress)
{
  JNIEnv * env = jni::GetEnv();
  jni::TScopedLocalRef const jProgress(env, ToJavaFirstGoalProgress(env, progress));
  env->CallVoidMethod(*listener,
                      jni::GetMethodID(env, *listener, "onFirstGoalProgressChanged",
                                       "(Lapp/organicmaps/sdk/maplayer/streetpixels/FirstGoalProgress;)V"),
                      jProgress.get());
}

static jobject ToJavaAreaMilestonePresentation(JNIEnv * env, street_pixels::AreaMilestonePresentation const & presentation)
{
  static jclass const presentationClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/maplayer/streetpixels/AreaMilestonePresentation");
  static jmethodID const ctor =
      jni::GetConstructorID(env, presentationClass, "(IJILjava/lang/String;Ljava/lang/String;)V");
  jni::TScopedLocalRef const jName(env, jni::ToJavaString(env, presentation.m_displayName));
  jni::TScopedLocalRef const jLine(env, jni::ToJavaString(env, presentation.m_competitionLine));
  return env->NewObject(presentationClass, ctor, static_cast<jint>(presentation.m_threshold),
                        static_cast<jlong>(presentation.m_osmId), static_cast<jint>(presentation.m_compactIndex),
                        jName.get(), jLine.get());
}

static void CallAreaMilestoneCallback(std::shared_ptr<jobject> const & listener,
                                      std::optional<street_pixels::AreaMilestonePresentation> const & presentation)
{
  JNIEnv * env = jni::GetEnv();
  jobject javaObj = nullptr;
  if (presentation)
    javaObj = ToJavaAreaMilestonePresentation(env, *presentation);
  jni::TScopedLocalRef const jPresentation(env, javaObj);
  env->CallVoidMethod(*listener,
                      jni::GetMethodID(env, *listener, "onAreaMilestonePresentationChanged",
                                       "(Lapp/organicmaps/sdk/maplayer/streetpixels/AreaMilestonePresentation;)V"),
                      jPresentation.get());
}

static void CallFocusedAreaCallback(std::shared_ptr<jobject> const & listener, char const * method,
                                    street_pixels::FocusedAreaProgress const & progress)
{
  JNIEnv * env = jni::GetEnv();
  jni::TScopedLocalRef const jProgress(env, ToJavaFocusedAreaProgress(env, progress));
  env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener, method,
                                                 "(Lapp/organicmaps/sdk/maplayer/streetpixels/FocusedAreaProgress;)V"),
                      jProgress.get());
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
  auto & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  manager.SetFocusedAreaProgressListener(
      [globalListener](street_pixels::FocusedAreaProgress const & progress)
      {
        GetPlatform().RunTask(Platform::Thread::Gui, [globalListener, progress]()
        { CallFocusedAreaCallback(globalListener, "onFocusedAreaProgressChanged", progress); });
      });
  manager.SetExplorationAreaTapListener(
      [globalListener](street_pixels::FocusedAreaProgress const & progress)
      {
        GetPlatform().RunTask(Platform::Thread::Gui, [globalListener, progress]()
        { CallFocusedAreaCallback(globalListener, "onExplorationAreaTapped", progress); });
      });
  manager.SetFirstGoalProgressListener(
      [globalListener](street_pixels::FirstGoalProgress const & progress)
      {
        GetPlatform().RunTask(Platform::Thread::Gui, [globalListener, progress]()
        { CallFirstGoalCallback(globalListener, progress); });
      });
  manager.SetAreaMilestonePresentationListener(
      [globalListener](std::optional<street_pixels::AreaMilestonePresentation> const & presentation)
      {
        GetPlatform().RunTask(Platform::Thread::Gui, [globalListener, presentation]()
        { CallAreaMilestoneCallback(globalListener, presentation); });
      });
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeRemoveListener(JNIEnv * env,
                                                                                                           jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  g_framework->SetStreetPixelsListener(nullptr);
  auto & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  manager.SetFocusedAreaProgressListener(nullptr);
  manager.SetExplorationAreaTapListener(nullptr);
  manager.SetFirstGoalProgressListener(nullptr);
  manager.SetAreaMilestonePresentationListener(nullptr);
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
  static_cast<void>(countryId);
  auto & native = *g_framework->NativeFramework();
  native.RefreshStreetPixelsFocusFromViewport();
  return ToJavaFocusedAreaProgress(env, native.GetStreetPixelsManager().GetFocusedAreaProgress());
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeSelectFocusedAreaAtLatLon(
    JNIEnv * env, jclass clazz, jdouble lat, jdouble lon, jstring countryId)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  static_cast<void>(countryId);
  auto & native = *g_framework->NativeFramework();
  native.SelectStreetPixelsFocusAt(mercator::FromLatLon(lat, lon));
  return ToJavaFocusedAreaProgress(env, native.GetStreetPixelsManager().GetFocusedAreaProgress());
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeGetFirstGoalProgress(JNIEnv * env,
                                                                                              jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  return ToJavaFirstGoalProgress(env, manager.GetFirstGoalProgress());
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeGetCurrentAreaMilestonePresentation(
    JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  auto const presentation = manager.GetCurrentAreaMilestonePresentation();
  if (!presentation)
    return nullptr;
  return ToJavaAreaMilestonePresentation(env, *presentation);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeAcknowledgeAreaMilestonePresentation(
    JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  manager.AcknowledgeAreaMilestonePresentation();
}
}
