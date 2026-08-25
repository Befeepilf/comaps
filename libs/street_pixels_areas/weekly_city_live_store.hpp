#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

struct sqlite3;

namespace street_pixels
{
struct CompetitionWeeklyCityQuery
{
  int64_t m_cityOsmId = 0;
  int64_t m_weekId = 0;
  int64_t m_weekEndUnix = 0;
  int64_t m_remainingSeconds = 0;
  int64_t m_newLiveCount = 0;
  bool m_usedUtcFallback = true;
};

struct WeeklyCityLiveCountRow
{
  int64_t m_cityOsmId = 0;
  int64_t m_weekId = 0;
  int64_t m_newLiveCount = 0;
};

class WeeklyCityLiveStore
{
public:
  static WeeklyCityLiveStore & Instance();

  WeeklyCityLiveStore(std::string dbPath = {});
  ~WeeklyCityLiveStore();

  WeeklyCityLiveStore(WeeklyCityLiveStore const &) = delete;
  WeeklyCityLiveStore & operator=(WeeklyCityLiveStore const &) = delete;

  static std::string DefaultDbPath();

  void RecordFirstLive(std::vector<int64_t> const & cityOsmIds, int64_t nowUnix);
  CompetitionWeeklyCityQuery Query(int64_t cityOsmId, int64_t nowUnix) const;
  std::vector<WeeklyCityLiveCountRow> ListCurrentWeekCounts(int64_t nowUnix) const;

  void SetCityIanaTz(int64_t cityOsmId, std::string const & ianaTz);
  std::string GetCityIanaTz(int64_t cityOsmId) const;

  void Reopen(std::string const & dbPath);

private:
  bool EnsureOpen() const;
  void CloseDb() const;
  void InitSchema() const;
  std::string LoadTzUnlocked(int64_t cityOsmId) const;
  int64_t LoadCountUnlocked(int64_t cityOsmId, int64_t weekId) const;
  CompetitionWeeklyCityQuery QueryUnlocked(int64_t cityOsmId, int64_t nowUnix) const;

  std::string m_dbPath;
  mutable sqlite3 * m_db = nullptr;
  mutable std::mutex m_mutex;
};
}  // namespace street_pixels
