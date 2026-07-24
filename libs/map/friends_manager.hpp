#pragma once

#include "base/visitor.hpp"

#include <functional>
#include <string>
#include <vector>
#include <mutex>

struct FriendRecord
{
  std::string m_userId;
  std::string m_username;

  DECLARE_VISITOR(visitor(m_userId, "user_id"), visitor(m_username, "username"))

  bool operator==(FriendRecord const & other) const
  {
    return m_userId == other.m_userId && m_username == other.m_username;
  }
};

struct FriendsLists
{
  std::vector<FriendRecord> m_accepted;
  std::vector<FriendRecord> m_incoming;
  std::vector<FriendRecord> m_outgoing;

  DECLARE_VISITOR(visitor(m_accepted, "accepted"), visitor(m_incoming, "incoming"), visitor(m_outgoing, "outgoing"))

  bool operator==(FriendsLists const & other) const
  {
    return m_accepted == other.m_accepted && m_incoming == other.m_incoming && m_outgoing == other.m_outgoing;
  }
};

class FriendsManager
{
public:
  class Subscriber
  {
  public:
    virtual ~Subscriber() = default;
    virtual void OnListsUpdated() {}
    virtual void OnSignupResult(bool success) {}
    virtual void OnUsernameChanged(bool success) {}
    virtual void OnActionResult(bool success) {}
    virtual void OnDeleteAccountResult(bool success) {}
    virtual void OnExportAccountResult(bool success, std::string const & json) {}
  };

  FriendsManager();

  bool LoadCache();
  bool SaveCache() const;
  bool EnsureCacheLoaded();

  std::string GetListsJson() const;
  FriendsLists const & GetLists() const { return m_lists; }

  void AddSubscriber(Subscriber * sub);
  void RemoveSubscriber(Subscriber * sub);

  void Refresh();
  void Signup(std::string const & username);
  void ChangeUsername(std::string const & username);

  using SearchCallback = std::function<void(std::vector<FriendRecord> const &)>;
  void SearchByUsername(std::string const & query, SearchCallback const & callback);

  void SendRequest(std::string const & userId);
  void AcceptRequest(std::string const & userId);
  void CancelRequest(std::string const & userId);

  void DeleteAccount();
  using ExportCallback = std::function<void(bool success, std::string const & json)>;
  void ExportAccount(ExportCallback const & callback);

private:
  void ClearLocalAccountData();
  std::string GetCacheFilePath() const;

  FriendsLists m_lists;
  bool m_cacheLoaded = false;

  mutable std::mutex m_subscribersMutex;
  std::vector<Subscriber *> m_subscribers;
};
