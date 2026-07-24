#include <jni.h>

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "map/friends_manager.hpp"

static FriendsManager g_friends;

namespace
{
jclass g_FriendClazz = nullptr;
jmethodID g_FriendCtor = nullptr;

jclass g_FriendsPayloadClazz = nullptr;
jmethodID g_FriendsPayloadCtor = nullptr;

jclass g_FriendsJClass = nullptr;

jobjectArray ToJavaFriendsArray(JNIEnv * env, std::vector<FriendRecord> const & v)
{
  if (!g_FriendClazz)
  {
    g_FriendClazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/friends/Friends$Friend");
    if (g_FriendClazz)
      g_FriendCtor = jni::GetConstructorID(env, g_FriendClazz, "(Ljava/lang/String;Ljava/lang/String;)V");
  }
  if (!g_FriendClazz || !g_FriendCtor)
    return nullptr;

  jobjectArray arr = env->NewObjectArray(static_cast<jsize>(v.size()), g_FriendClazz, nullptr);
  if (!arr)
    return nullptr;
  for (size_t i = 0; i < v.size(); ++i)
  {
    jni::TScopedLocalRef const uid(env, jni::ToJavaString(env, v[i].m_userId));
    jni::TScopedLocalRef const uname(env, jni::ToJavaString(env, v[i].m_username));
    jobject obj = env->NewObject(g_FriendClazz, g_FriendCtor, uid.get(), uname.get());
    if (!obj)
    {
      env->DeleteLocalRef(arr);
      return nullptr;
    }
    env->SetObjectArrayElement(arr, static_cast<jsize>(i), obj);
    env->DeleteLocalRef(obj);
  }
  return arr;
}

class JniSubscriber : public FriendsManager::Subscriber
{
public:
  void OnListsUpdated() override
  {
    if (!g_FriendsJClass)
      return;
    JNIEnv * env = jni::GetEnv();
    jmethodID const method = env->GetStaticMethodID(g_FriendsJClass, "onListsUpdated", "()V");
    if (method)
      env->CallStaticVoidMethod(g_FriendsJClass, method);
  }

  void OnSignupResult(bool success) override
  {
    if (!g_FriendsJClass)
      return;
    JNIEnv * env = jni::GetEnv();
    jmethodID const method = env->GetStaticMethodID(g_FriendsJClass, "onSignupResult", "(Z)V");
    if (method)
      env->CallStaticVoidMethod(g_FriendsJClass, method, static_cast<jboolean>(success));
  }

  void OnUsernameChanged(bool success) override
  {
    if (!g_FriendsJClass)
      return;
    JNIEnv * env = jni::GetEnv();
    jmethodID const method = env->GetStaticMethodID(g_FriendsJClass, "onUsernameChanged", "(Z)V");
    if (method)
      env->CallStaticVoidMethod(g_FriendsJClass, method, static_cast<jboolean>(success));
  }

  void OnActionResult(bool success) override
  {
    if (!g_FriendsJClass)
      return;
    JNIEnv * env = jni::GetEnv();
    jmethodID const method = env->GetStaticMethodID(g_FriendsJClass, "onActionResult", "(Z)V");
    if (method)
      env->CallStaticVoidMethod(g_FriendsJClass, method, static_cast<jboolean>(success));
  }

  void OnDeleteAccountResult(bool success) override
  {
    if (!g_FriendsJClass)
      return;
    JNIEnv * env = jni::GetEnv();
    jmethodID const method = env->GetStaticMethodID(g_FriendsJClass, "onDeleteAccountResult", "(Z)V");
    if (method)
      env->CallStaticVoidMethod(g_FriendsJClass, method, static_cast<jboolean>(success));
  }

  void OnExportAccountResult(bool success, std::string const & json) override
  {
    if (!g_FriendsJClass)
      return;
    JNIEnv * env = jni::GetEnv();
    jmethodID const method =
        env->GetStaticMethodID(g_FriendsJClass, "onExportAccountResult", "(ZLjava/lang/String;)V");
    if (method)
    {
      jni::TScopedLocalRef const jjson(env, jni::ToJavaString(env, json));
      env->CallStaticVoidMethod(g_FriendsJClass, method, static_cast<jboolean>(success), jjson.get());
    }
  }
};

static JniSubscriber g_subscriber;
}  // namespace

extern "C"
{
JNIEXPORT jobject JNICALL Java_app_organicmaps_sdk_friends_Friends_nativeGetLists(JNIEnv * env, jclass)
{
  (void)g_friends.EnsureCacheLoaded();
  if (!g_FriendsPayloadClazz)
  {
    g_FriendsPayloadClazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/friends/Friends$FriendsPayload");
    if (g_FriendsPayloadClazz)
    {
      g_FriendsPayloadCtor = jni::GetConstructorID(env, g_FriendsPayloadClazz,
                                                   "([Lapp/organicmaps/sdk/friends/Friends$Friend;[Lapp/organicmaps/sdk/friends/"
                                                   "Friends$Friend;[Lapp/organicmaps/sdk/friends/Friends$Friend;)V");
    }
  }
  if (!g_FriendsPayloadClazz || !g_FriendsPayloadCtor)
    return nullptr;

  auto const & lists = g_friends.GetLists();
  auto const toJavaOrEmpty = [&](std::vector<FriendRecord> const & vec) -> jobjectArray {
    jobjectArray j = ToJavaFriendsArray(env, vec);
    if (j)
      return j;
    return env->NewObjectArray(0, g_FriendClazz, nullptr);
  };
  jobjectArray accepted = toJavaOrEmpty(lists.m_accepted);
  jobjectArray incoming = toJavaOrEmpty(lists.m_incoming);
  jobjectArray outgoing = toJavaOrEmpty(lists.m_outgoing);
  return env->NewObject(g_FriendsPayloadClazz, g_FriendsPayloadCtor, accepted, incoming, outgoing);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_friends_Friends_nativeRefresh(JNIEnv *, jclass) { g_friends.Refresh(); }

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_friends_Friends_nativeSignup(JNIEnv * env, jclass, jstring username)
{
  g_friends.Signup(jni::ToNativeString(env, username));
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_friends_Friends_nativeChangeUsername(JNIEnv * env, jclass, jstring username)
{
  g_friends.ChangeUsername(jni::ToNativeString(env, username));
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_friends_Friends_nativeSearchByUsername(JNIEnv * env, jclass,
                                                                                           jstring query, jobject callback)
{
  if (!callback)
    return;
  auto const q = jni::ToNativeString(env, query);
  auto const globalCallback = jni::make_global_ref(callback);
  g_friends.SearchByUsername(q, [globalCallback](std::vector<FriendRecord> const & results)
  {
    JNIEnv * env = jni::GetEnv();
    jobjectArray const rawArr = ToJavaFriendsArray(env, results);
    jni::TScopedLocalRef const arrRef(env, rawArr);
    jni::TScopedLocalClassRef const callbackInterfaceRef(
        env, env->FindClass("app/organicmaps/sdk/friends/Friends$SearchCallback"));
    jmethodID method = nullptr;
    if (callbackInterfaceRef.get())
      method = env->GetMethodID(callbackInterfaceRef.get(), "onSearchResult",
                                "([Lapp/organicmaps/sdk/friends/Friends$Friend;)V");
    if (!method)
    {
      jni::TScopedLocalClassRef const objClass(env, env->GetObjectClass(*globalCallback));
      if (objClass.get())
        method = env->GetMethodID(objClass.get(), "onSearchResult",
                                  "([Lapp/organicmaps/sdk/friends/Friends$Friend;)V");
    }
    if (method)
      env->CallVoidMethod(*globalCallback, method, arrRef.get());
    else
      LOG(LERROR, ("JNI: Could not find onSearchResult on Friends search callback"));
  });
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_friends_Friends_nativeSendRequest(JNIEnv * env, jclass, jstring userId)
{
  g_friends.SendRequest(jni::ToNativeString(env, userId));
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_friends_Friends_nativeAcceptRequest(JNIEnv * env, jclass,
                                                                                    jstring userId)
{
  g_friends.AcceptRequest(jni::ToNativeString(env, userId));
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_friends_Friends_nativeCancelRequest(JNIEnv * env, jclass,
                                                                                    jstring userId)
{
  g_friends.CancelRequest(jni::ToNativeString(env, userId));
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_friends_Friends_nativeSubscribe(JNIEnv * env, jclass)
{
  if (!g_FriendsJClass)
    g_FriendsJClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/friends/Friends");
  g_friends.AddSubscriber(&g_subscriber);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_friends_Friends_nativeUnsubscribe(JNIEnv *, jclass)
{
  g_friends.RemoveSubscriber(&g_subscriber);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_friends_Friends_nativeDeleteAccount(JNIEnv *, jclass)
{
  g_friends.DeleteAccount();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_friends_Friends_nativeExportAccount(JNIEnv * env, jclass, jobject callback)
{
  if (!callback)
    return;
  auto const globalCallback = jni::make_global_ref(callback);
  g_friends.ExportAccount([globalCallback](bool success, std::string const & json)
  {
    JNIEnv * env = jni::GetEnv();
    jni::TScopedLocalClassRef const callbackInterfaceRef(
        env, env->FindClass("app/organicmaps/sdk/friends/Friends$ExportCallback"));
    jmethodID method = nullptr;
    if (callbackInterfaceRef.get())
      method = env->GetMethodID(callbackInterfaceRef.get(), "onExportResult", "(ZLjava/lang/String;)V");
    if (!method)
    {
      jni::TScopedLocalClassRef const objClass(env, env->GetObjectClass(*globalCallback));
      if (objClass.get())
        method = env->GetMethodID(objClass.get(), "onExportResult", "(ZLjava/lang/String;)V");
    }
    jni::TScopedLocalRef const jjson(env, jni::ToJavaString(env, json));
    if (method)
      env->CallVoidMethod(*globalCallback, method, static_cast<jboolean>(success), jjson.get());
  });
}
}
