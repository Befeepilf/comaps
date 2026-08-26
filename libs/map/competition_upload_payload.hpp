#pragma once

#include "base/visitor.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct CompetitionUploadArea
{
  uint64_t m_areaOsmId = 0;
  double m_ownershipScore = 0.0;
  double m_liveCoveragePct = 0.0;
  bool m_eligible = false;

  DECLARE_VISITOR(visitor(m_areaOsmId, "area_osm_id"), visitor(m_ownershipScore, "ownership_score"),
                  visitor(m_liveCoveragePct, "live_coverage_pct"), visitor(m_eligible, "eligible"))
};

struct CompetitionUploadWeeklyCity
{
  int64_t m_cityOsmId = 0;
  int64_t m_newLiveCount = 0;

  DECLARE_VISITOR(visitor(m_cityOsmId, "city_osm_id"), visitor(m_newLiveCount, "new_live_count"))
};

struct CompetitionUploadPayload
{
  std::string m_profileId;
  std::string m_nickname;
  int64_t m_mapDataVersion = 0;
  int m_scoreCalcVersion = 1;
  int64_t m_lastUpdateUnix = 0;
  std::vector<CompetitionUploadArea> m_areas;
  std::vector<CompetitionUploadWeeklyCity> m_weeklyCities;

  DECLARE_VISITOR(visitor(m_profileId, "profile_id"), visitor(m_nickname, "nickname"),
                  visitor(m_mapDataVersion, "map_data_version"), visitor(m_scoreCalcVersion, "score_calc_version"),
                  visitor(m_lastUpdateUnix, "last_update_unix"), visitor(m_areas, "areas"),
                  visitor(m_weeklyCities, "weekly_cities"))
};

bool CompetitionUploadPayloadIsEmpty(CompetitionUploadPayload const & payload);
std::string SerializeCompetitionUploadPayload(CompetitionUploadPayload const & payload);
