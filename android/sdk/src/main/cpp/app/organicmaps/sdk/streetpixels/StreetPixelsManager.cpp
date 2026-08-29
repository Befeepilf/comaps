#include <jni.h>
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/platform/AndroidPlatform.hpp"

#include "street_pixels_areas/completion_card.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/focus_selection_engine.hpp"
#include "street_pixels_areas/focused_area_progress.hpp"

#include "map/area_milestone_presentation.hpp"
#include "map/competition_hint.hpp"
#include "map/competition_snapshot.hpp"
#include "map/first_goal.hpp"
#include "map/identity_store.hpp"

#include "street_pixels_areas/competition_presentation.hpp"

#include "platform/local_country_file.hpp"
#include "platform/location.hpp"
#include "platform/platform.hpp"

#include "storage/storage_defines.hpp"

#include "geometry/point2d.hpp"
#include "geometry/mercator.hpp"

#include <optional>
#include <vector>

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
      jni::GetConstructorID(env, presentationClass, "(IJILjava/lang/String;Ljava/lang/String;Z)V");
  jni::TScopedLocalRef const jName(env, jni::ToJavaString(env, presentation.m_displayName));
  jni::TScopedLocalRef const jLine(env, jni::ToJavaString(env, presentation.m_competitionLine));
  return env->NewObject(presentationClass, ctor, static_cast<jint>(presentation.m_threshold),
                        static_cast<jlong>(presentation.m_osmId), static_cast<jint>(presentation.m_compactIndex),
                        jName.get(), jLine.get(), static_cast<jboolean>(presentation.m_debugPreview));
}

static void CallCompetitionHintReady(std::shared_ptr<jobject> const & listener)
{
  JNIEnv * env = jni::GetEnv();
  env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener, "onCompetitionHintReady", "()V"));
}

static jobject ToJavaCompetitionRankingRow(JNIEnv * env, street_pixels::CompetitionRankingEntry const & row,
                                           std::string const & selfNickname)
{
  static jclass const rowClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/maplayer/streetpixels/CompetitionRankingRow");
  static jmethodID const ctor = jni::GetConstructorID(env, rowClass, "(ILjava/lang/String;DZ)V");
  std::string const name = street_pixels::RankingDisplayName(row.m_nickname, row.m_isCurrentUser, selfNickname);
  jni::TScopedLocalRef const jName(env, jni::ToJavaString(env, name));
  return env->NewObject(rowClass, ctor, static_cast<jint>(row.m_rank), jName.get(),
                        static_cast<jdouble>(row.m_decayedScore), static_cast<jboolean>(row.m_isCurrentUser));
}

static jobject ToJavaCompetitionAreaChrome(JNIEnv * env, street_pixels::CompetitionAreaChrome const & chrome)
{
  static jclass const chromeClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/maplayer/streetpixels/CompetitionAreaChrome");
  static jclass const rowClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/maplayer/streetpixels/CompetitionRankingRow");
  static jmethodID const ctor = jni::GetConstructorID(
      env, chromeClass,
      "(ZZZZLjava/lang/String;Ljava/lang/String;[Lapp/organicmaps/sdk/maplayer/streetpixels/CompetitionRankingRow;DDZZ)V");
  std::string const self = IdentityStore::GetUsername();
  jni::TScopedLocalRef const jBoss(env, jni::ToJavaString(env, chrome.m_bossLine));
  jni::TScopedLocalRef const jGap(env, jni::ToJavaString(env, chrome.m_gapLine));
  jobjectArray rows = env->NewObjectArray(static_cast<jsize>(chrome.m_rankingRows.size()), rowClass, nullptr);
  for (size_t i = 0; i < chrome.m_rankingRows.size(); ++i)
  {
    jni::TScopedLocalRef const jRow(env, ToJavaCompetitionRankingRow(env, chrome.m_rankingRows[i], self));
    env->SetObjectArrayElement(rows, static_cast<jsize>(i), jRow.get());
  }
  jni::TScopedLocalRef const jRows(env, rows);
  return env->NewObject(chromeClass, ctor, static_cast<jboolean>(chrome.m_offline),
                        static_cast<jboolean>(chrome.m_stale), static_cast<jboolean>(chrome.m_unclaimed),
                        static_cast<jboolean>(chrome.m_contested), jBoss.get(), jGap.get(), jRows.get(),
                        static_cast<jdouble>(chrome.m_localOwnershipScore),
                        static_cast<jdouble>(chrome.m_personalCompletionFraction),
                        static_cast<jboolean>(chrome.m_localEligible),
                        static_cast<jboolean>(chrome.m_localIsBoss));
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
  manager.SetCompetitionHintReadyHandler(
      [globalListener]()
      {
        GetPlatform().RunTask(Platform::Thread::Gui, [globalListener]()
        { CallCompetitionHintReady(globalListener); });
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
  manager.SetCompetitionHintReadyHandler(nullptr);
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

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeDebugTriggerAchievementPresentations(
    JNIEnv *, jclass)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  g_framework->NativeFramework()->GetStreetPixelsManager().DebugTriggerAchievementPresentations();
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

static jobject ToJavaCompletionCardModel(JNIEnv * env, street_pixels::CompletionCardModel const & model)
{
  static jclass const modelClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/maplayer/streetpixels/CompletionCardModel");
  static jmethodID const ctor = jni::GetConstructorID(
      env, modelClass,
      "(Ljava/lang/String;Ljava/lang/String;[F[F[ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/"
      "String;)V");

  auto const projected = street_pixels::ProjectOutlineToPixels(
      model.m_outlineRings, street_pixels::kCompletionCardOutlineSize, street_pixels::kCompletionCardOutlineSize);
  size_t vertexCount = 0;
  for (auto const & ring : projected)
    vertexCount += ring.size();

  std::vector<jfloat> xs(vertexCount);
  std::vector<jfloat> ys(vertexCount);
  std::vector<jint> ringLengths(projected.size());
  size_t cursor = 0;
  for (size_t r = 0; r < projected.size(); ++r)
  {
    ringLengths[r] = static_cast<jint>(projected[r].size());
    for (auto const & p : projected[r])
    {
      xs[cursor] = static_cast<jfloat>(p.x);
      ys[cursor] = static_cast<jfloat>(p.y);
      ++cursor;
    }
  }

  jni::ScopedLocalRef<jfloatArray> const jXs(env, env->NewFloatArray(static_cast<jsize>(xs.size())));
  jni::ScopedLocalRef<jfloatArray> const jYs(env, env->NewFloatArray(static_cast<jsize>(ys.size())));
  jni::TScopedLocalIntArrayRef const jLengths(env, env->NewIntArray(static_cast<jsize>(ringLengths.size())));
  if (jXs.get() != nullptr && !xs.empty())
    env->SetFloatArrayRegion(jXs.get(), 0, static_cast<jsize>(xs.size()), xs.data());
  if (jYs.get() != nullptr && !ys.empty())
    env->SetFloatArrayRegion(jYs.get(), 0, static_cast<jsize>(ys.size()), ys.data());
  if (jLengths.get() != nullptr && !ringLengths.empty())
    env->SetIntArrayRegion(jLengths.get(), 0, static_cast<jsize>(ringLengths.size()), ringLengths.data());

  jni::TScopedLocalRef const jName(env, jni::ToJavaString(env, model.m_areaDisplayName));
  jni::TScopedLocalRef const jHeadline(env, jni::ToJavaString(env, model.m_headline));
  jni::TScopedLocalRef const jNick(env, model.m_nickname ? jni::ToJavaString(env, *model.m_nickname) : nullptr);
  jni::TScopedLocalRef const jDate(env, model.m_completedDate ? jni::ToJavaString(env, *model.m_completedDate)
                                                             : nullptr);
  jni::TScopedLocalRef const jBrand(env, jni::ToJavaString(env, model.m_branding));
  jni::TScopedLocalRef const jComp(env, jni::ToJavaString(env, model.m_competitionLine));
  return env->NewObject(modelClass, ctor, jName.get(), jHeadline.get(), jXs.get(), jYs.get(), jLengths.get(),
                        jNick.get(), jDate.get(), jBrand.get(), jComp.get());
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeGetCurrentCompletionCard(
    JNIEnv * env, jclass clazz, jboolean recordGenerated)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  auto const model = manager.GetCompletionCardForCurrentPresentation(static_cast<bool>(recordGenerated));
  if (!model)
    return nullptr;
  return ToJavaCompletionCardModel(env, *model);
}

static jobject ToJavaCompletionCardSharePayload(JNIEnv * env, street_pixels::CompletionCardSharePayload const & payload)
{
  static jclass const payloadClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/maplayer/streetpixels/CompletionCardSharePayload");
  static jmethodID const ctor =
      jni::GetConstructorID(env, payloadClass, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  jni::TScopedLocalRef const jPath(env, jni::ToJavaString(env, payload.m_path));
  jni::TScopedLocalRef const jMime(env, jni::ToJavaString(env, payload.m_mimeType));
  jni::TScopedLocalRef const jText(env, jni::ToJavaString(env, payload.m_text));
  return env->NewObject(payloadClass, ctor, jPath.get(), jMime.get(), jText.get());
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativePrepareCompletionCardShare(
    JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  auto const payload = manager.PrepareCompletionCardShare();
  if (!payload)
    return nullptr;
  return ToJavaCompletionCardSharePayload(env, *payload);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeRecordCompletionCardShareInitiated(
    JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  manager.RecordCompletionCardShareInitiated();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeAcknowledgeCompetitionHint(JNIEnv *, jclass)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  manager.AcknowledgeCompetitionHint();
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativePeekCompetitionHintText(JNIEnv * env, jclass)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto const & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  auto const text = manager.PeekCompetitionHintText();
  if (!text)
    return nullptr;
  return jni::ToJavaString(env, *text);
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeTryConsumeOvertakingHint(
    JNIEnv * env, jclass, jboolean routingFollowing)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  auto const text = manager.TryConsumeOvertakingHint(static_cast<bool>(routingFollowing));
  if (!text)
    return nullptr;
  return jni::ToJavaString(env, *text);
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeGetCompetitionAreaChrome(JNIEnv * env, jclass,
                                                                                                 jlong osmId)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto const & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  return ToJavaCompetitionAreaChrome(env, manager.GetCompetitionAreaChrome(static_cast<uint64_t>(osmId)));
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_sdk_maplayer_streetpixels_StreetPixelsManager_nativeRequestCompetitionAreaSnapshot(JNIEnv * env,
                                                                                                       jclass,
                                                                                                       jlong osmId)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto & manager = g_framework->NativeFramework()->GetStreetPixelsManager();
  auto const result = manager.RequestCompetitionAreaSnapshot(static_cast<uint64_t>(osmId));
  return ToJavaCompetitionAreaChrome(env, result.m_chrome);
}
}
