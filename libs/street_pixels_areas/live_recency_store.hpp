#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct sqlite3;

namespace street_pixels
{
class LiveRecencyStore
{
public:
  static LiveRecencyStore & Instance();

  LiveRecencyStore(std::string dbPath = {});
  ~LiveRecencyStore();

  LiveRecencyStore(LiveRecencyStore const &) = delete;
  LiveRecencyStore & operator=(LiveRecencyStore const &) = delete;

  static std::string DefaultDbPath();

  void SeedEverLive(std::vector<int64_t> const & ids, int64_t consentUnixSec);
  void TouchLiveVisits(std::vector<int64_t> const & ids, int64_t nowUnixSec);

  std::optional<int64_t> GetLastLiveVisit(int64_t healpixId) const;
  std::vector<std::optional<int64_t>> GetLastLiveVisits(std::vector<int64_t> const & ids) const;

  void ClearAll();
  void Reopen(std::string const & dbPath);

private:
  bool EnsureOpen() const;
  void CloseDb() const;
  void InitSchema() const;
  void RunVisitBatch(char const * sql, std::vector<int64_t> const & ids, int64_t unixSec);
  std::optional<int64_t> LoadVisitUnlocked(int64_t healpixId) const;

  std::string m_dbPath;
  mutable sqlite3 * m_db = nullptr;
  mutable std::mutex m_mutex;
};
}  // namespace street_pixels
