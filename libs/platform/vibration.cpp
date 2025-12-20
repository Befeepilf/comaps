#include "platform/vibration.hpp"
#if defined(OMIM_OS_ANDROID)
#include <jni.h>
#include <vector>

#include "android/sdk/src/main/cpp/app/organicmaps/sdk/core/jni_helper.hpp"
#include "android/sdk/src/main/cpp/app/organicmaps/sdk/platform/AndroidPlatform.hpp"

namespace platform
{
void Vibrate(uint32_t durationMs)
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return;

  static jmethodID const vibrateId = jni::GetStaticMethodID(env, g_utilsClazz, "vibrate", "(Landroid/content/Context;J)V");
  if (vibrateId == nullptr)
    return;

  jobject context = android::Platform::Instance().GetContext();
  env->CallStaticVoidMethod(g_utilsClazz, vibrateId, context, static_cast<jlong>(durationMs));
}

void VibratePattern(uint32_t const * durations, uint32_t const * delays, size_t count)
{
  JNIEnv * env = jni::GetEnv();
  if (env == nullptr)
    return;

  static jmethodID const vibratePatternId = jni::GetStaticMethodID(env, g_utilsClazz, "vibratePattern", "(Landroid/content/Context;[J[J)V");
  if (vibratePatternId == nullptr)
    return;

  jobject context = android::Platform::Instance().GetContext();
  jlongArray durationsArray = env->NewLongArray(static_cast<jsize>(count));
  jlongArray delaysArray = env->NewLongArray(static_cast<jsize>(count));

  if (durationsArray == nullptr || delaysArray == nullptr)
    return;

  std::vector<jlong> durationsVec(durations, durations + count);
  std::vector<jlong> delaysVec(delays, delays + count);

  env->SetLongArrayRegion(durationsArray, 0, static_cast<jsize>(count), durationsVec.data());
  env->SetLongArrayRegion(delaysArray, 0, static_cast<jsize>(count), delaysVec.data());

  env->CallStaticVoidMethod(g_utilsClazz, vibratePatternId, context, durationsArray, delaysArray);

  env->DeleteLocalRef(durationsArray);
  env->DeleteLocalRef(delaysArray);
}
}  // namespace platform
#else

namespace platform
{
void Vibrate(uint32_t /*durationMs*/)
{
  // Vibration is not supported on this platform.
}

void VibratePattern(uint32_t const * /*durations*/, uint32_t const * /*delays*/, size_t /*count*/)
{
  // Vibration is not supported on this platform.
}
}  // namespace platform

#endif  // OMIM_OS_ANDROID