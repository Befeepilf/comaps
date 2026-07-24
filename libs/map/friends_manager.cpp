#include "base/logging.hpp"

#include "map/friends_manager.hpp"
#include "map/backend_config.hpp"
#include "map/identity_store.hpp"

#include "platform/settings.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/serdes_json.hpp"
#include "coding/url.hpp"
#include "coding/writer.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

namespace
{
constexpr char kFriendsCacheFile[] = "friends_cache.json";

struct UsernameBody
{
  std::string m_username;
  DECLARE_VISITOR(visitor(m_username, "username"))
};

std::string UsernameToJson(std::string const & username)
{
  UsernameBody body;
  body.m_username = username;
  std::string json;
  using Sink = MemWriter<std::string>;
  Sink sink(json);
  coding::SerializerJson<Sink> ser(sink);
  ser(body);
  return json;
}

void AddAuthHeaders(platform::HttpClient & req)
{
  req.SetRawHeader("X-Device-Id", IdentityStore::GetOrCreateDeviceId());
  if (IdentityStore::HasUsername())
    req.SetRawHeader("X-Username", IdentityStore::GetUsername());
}
}  // namespace

FriendsManager::FriendsManager() = default;

std::string FriendsManager::GetCacheFilePath() const { return GetPlatform().WritablePathForFile(kFriendsCacheFile); }

bool FriendsManager::LoadCache()
{
  std::string json;
  try
  {
    FileReader reader(GetCacheFilePath());
    uint64_t const size = reader.Size();
    if (size == 0)
      return false;
    json.resize(static_cast<size_t>(size));
    reader.Read(0, &json[0], json.size());
  }
  catch (...)
  {
    return false;
  }

  if (json.empty())
    return false;

  try
  {
    coding::DeserializerJson des(json);
    des(m_lists);
    m_cacheLoaded = true;
    return true;
  }
  catch (...)
  {
    LOG(LWARNING, ("Failed to parse friends cache"));
    return false;
  }
}

bool FriendsManager::EnsureCacheLoaded()
{
  if (m_cacheLoaded)
    return true;
  return LoadCache();
}

bool FriendsManager::SaveCache() const
{
  try
  {
    FileWriter writer(GetCacheFilePath());
    coding::SerializerJson<FileWriter> ser(writer);
    ser(m_lists);
    return true;
  }
  catch (...)
  {
    LOG(LWARNING, ("Failed writing", GetCacheFilePath()));
    return false;
  }
}

std::string FriendsManager::GetListsJson() const
{
  std::string jsonStr;
  using Sink = MemWriter<std::string>;
  Sink sink(jsonStr);
  coding::SerializerJson<Sink> ser(sink);
  ser(m_lists);
  return jsonStr;
}

void FriendsManager::AddSubscriber(Subscriber * sub)
{
  std::lock_guard<std::mutex> lock(m_subscribersMutex);
  if (std::find(m_subscribers.begin(), m_subscribers.end(), sub) == m_subscribers.end())
    m_subscribers.push_back(sub);
}

void FriendsManager::RemoveSubscriber(Subscriber * sub)
{
  std::lock_guard<std::mutex> lock(m_subscribersMutex);
  m_subscribers.erase(std::remove(m_subscribers.begin(), m_subscribers.end(), sub), m_subscribers.end());
}

void FriendsManager::Refresh()
{
  GetPlatform().RunTask(Platform::Thread::Network, [this]()
  {
    std::string const url = backend::GetApiBaseUrl() + "/friends/list";
    platform::HttpClient request(url);
    AddAuthHeaders(request);
    std::string json;
    if (request.RunHttpRequest(json))
    {
      try
      {
        coding::DeserializerJson des(json);
        FriendsLists lists;
        des(lists);
        GetPlatform().RunTask(Platform::Thread::Gui, [this, lists = std::move(lists)]() mutable
        {
          m_lists = std::move(lists);
          m_cacheLoaded = true;
          SaveCache();

          std::lock_guard<std::mutex> lock(m_subscribersMutex);
          for (auto * sub : m_subscribers)
            sub->OnListsUpdated();
        });
        return;
      }
      catch (...) {}
    }
    LOG(LWARNING, ("Failed to refresh friends list"));
  });
}

void FriendsManager::Signup(std::string const & username)
{
  GetPlatform().RunTask(Platform::Thread::Network, [this, username]()
  {
    std::string const url = backend::GetApiBaseUrl() + "/signup";
    platform::HttpClient request(url);
    AddAuthHeaders(request);
    request.SetBodyData(UsernameToJson(username), "application/json");
    std::string response;
    bool const success = request.RunHttpRequest(response);
    GetPlatform().RunTask(Platform::Thread::Gui, [this, success, username]()
    {
      if (success)
        IdentityStore::SetUsername(username);

      std::lock_guard<std::mutex> lock(m_subscribersMutex);
      for (auto * sub : m_subscribers)
        sub->OnSignupResult(success);
    });
  });
}

void FriendsManager::ChangeUsername(std::string const & username)
{
  GetPlatform().RunTask(Platform::Thread::Network, [this, username]()
  {
    std::string const url = backend::GetApiBaseUrl() + "/update_username";
    platform::HttpClient request(url);
    AddAuthHeaders(request);
    request.SetBodyData(UsernameToJson(username), "application/json");
    std::string response;
    bool const success = request.RunHttpRequest(response);
    GetPlatform().RunTask(Platform::Thread::Gui, [this, success, username]()
    {
      if (success)
        IdentityStore::SetUsername(username);

      std::lock_guard<std::mutex> lock(m_subscribersMutex);
      for (auto * sub : m_subscribers)
        sub->OnUsernameChanged(success);
    });
  });
}

void FriendsManager::SearchByUsername(std::string const & query, SearchCallback const & callback)
{
  GetPlatform().RunTask(Platform::Thread::Network, [query, callback]()
  {
    std::string const url = backend::GetApiBaseUrl() + "/friends/search?query=" + url::UrlEncode(query);
    platform::HttpClient request(url);
    AddAuthHeaders(request);
    std::string json;
    std::vector<FriendRecord> results;
    if (request.RunHttpRequest(json))
    {
      try
      {
        coding::DeserializerJson des(json);
        des(results);
      }
      catch (...) {}
    }
    GetPlatform().RunTask(Platform::Thread::Gui, [results = std::move(results), callback]()
    {
      callback(results);
    });
  });
}

void FriendsManager::SendRequest(std::string const & userId)
{
  GetPlatform().RunTask(Platform::Thread::Network, [this, userId]()
  {
    std::string const url =
        backend::GetApiBaseUrl() + "/friends/request?to_user_id=" + url::UrlEncode(userId);
    platform::HttpClient request(url);
    AddAuthHeaders(request);
    request.SetBodyData("{}", "application/json");
    std::string response;
    bool const success = request.RunHttpRequest(response);
    GetPlatform().RunTask(Platform::Thread::Gui, [this, success]()
    {
      std::lock_guard<std::mutex> lock(m_subscribersMutex);
      for (auto * sub : m_subscribers)
        sub->OnActionResult(success);
    });
  });
}

void FriendsManager::AcceptRequest(std::string const & userId)
{
  GetPlatform().RunTask(Platform::Thread::Network, [this, userId]()
  {
    std::string const url =
        backend::GetApiBaseUrl() + "/friends/accept?from_user_id=" + url::UrlEncode(userId);
    platform::HttpClient request(url);
    AddAuthHeaders(request);
    request.SetBodyData("{}", "application/json");
    std::string response;
    bool const success = request.RunHttpRequest(response);
    GetPlatform().RunTask(Platform::Thread::Gui, [this, success]()
    {
      std::lock_guard<std::mutex> lock(m_subscribersMutex);
      for (auto * sub : m_subscribers)
        sub->OnActionResult(success);
    });
  });
}

void FriendsManager::CancelRequest(std::string const & userId)
{
  GetPlatform().RunTask(Platform::Thread::Network, [this, userId]()
  {
    std::string const url =
        backend::GetApiBaseUrl() + "/friends/cancel?user_id=" + url::UrlEncode(userId);
    platform::HttpClient request(url);
    AddAuthHeaders(request);
    request.SetBodyData("{}", "application/json");
    std::string response;
    bool const success = request.RunHttpRequest(response);
    GetPlatform().RunTask(Platform::Thread::Gui, [this, success]()
    {
      std::lock_guard<std::mutex> lock(m_subscribersMutex);
      for (auto * sub : m_subscribers)
        sub->OnActionResult(success);
    });
  });
}

void FriendsManager::ClearLocalAccountData()
{
  IdentityStore::ClearUsername();
  IdentityStore::SetExploreConsent(false);
  settings::Set("Explore.SyncEnabled", false);
  settings::Set("Explore.FriendVisibilityEnabled", false);
  m_lists = {};
  m_cacheLoaded = false;
  GetPlatform().RemoveFileIfExists(GetCacheFilePath());
}

void FriendsManager::DeleteAccount()
{
  GetPlatform().RunTask(Platform::Thread::Network, [this]()
  {
    std::string const url = backend::GetApiBaseUrl() + "/account";
    platform::HttpClient request(url);
    AddAuthHeaders(request);
    request.SetHttpMethod("DELETE");
    std::string response;
    bool const success = request.RunHttpRequest(response);
    GetPlatform().RunTask(Platform::Thread::Gui, [this, success]()
    {
      if (success)
        ClearLocalAccountData();

      std::lock_guard<std::mutex> lock(m_subscribersMutex);
      for (auto * sub : m_subscribers)
        sub->OnDeleteAccountResult(success);
    });
  });
}

void FriendsManager::ExportAccount(ExportCallback const & callback)
{
  GetPlatform().RunTask(Platform::Thread::Network, [callback]()
  {
    std::string const url = backend::GetApiBaseUrl() + "/account/export";
    platform::HttpClient request(url);
    AddAuthHeaders(request);
    std::string json;
    bool const success = request.RunHttpRequest(json);
    GetPlatform().RunTask(Platform::Thread::Gui, [callback, success, json = std::move(json)]()
    {
      callback(success, json);
    });
  });
}
