#pragma once

#include "street_pixels_areas/competition_presentation.hpp"

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace street_pixels
{
using CompetitionGetFn =
    std::function<int(std::string const & url, std::string & response,
                      std::vector<std::pair<std::string, std::string>> const & headers)>;

bool ParseAreaSnapshotJson(std::string const & json, CompetitionAreaSnapshot & out);
bool ParseWeeklyBoardJson(std::string const & json, CompetitionWeeklyBoard & out);

void SetCompetitionGetFnForTesting(CompetitionGetFn fn);
int GetCompetitionJson(std::string const & url, std::string & response);

CompetitionMapMode GetCompetitionMapMode();
void SetCompetitionMapMode(CompetitionMapMode mode);

std::optional<CompetitionAreaSnapshot> LastAreaSnapshot();
void ClearCompetitionSnapshotCacheForTesting();

std::optional<CompetitionWeeklyBoard> LastWeeklyBoard();
void ClearCompetitionWeeklyCacheForTesting();

struct FetchAreaSnapshotResult
{
  bool m_didGet = false;
  int m_httpStatus = 0;
  std::string m_url;
  CompetitionAreaChrome m_chrome;
  std::optional<CompetitionAreaSnapshot> m_snapshot;
};

FetchAreaSnapshotResult FetchAreaSnapshot(int64_t areaOsmId, std::string const & profileId);

struct FetchWeeklyBoardResult
{
  bool m_didGet = false;
  int m_httpStatus = 0;
  std::string m_url;
  CompetitionWeeklyChrome m_chrome;
  std::optional<CompetitionWeeklyBoard> m_board;
};

FetchWeeklyBoardResult FetchWeeklyBoard(int64_t cityOsmId, std::string const & profileId);

bool ShouldEmitOvertakingHint(uint64_t nowUnix);
void MarkOvertakingHintEmitted(uint64_t nowUnix);
void ClearOvertakingHintForTesting();
}  // namespace street_pixels
