#include "street_pixels_areas/area_completion_cache.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/sample_centres.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace street_pixels
{
namespace
{
m2::PointD CentreForSlot(std::vector<int64_t> const & universeAscendingNest,
                         std::vector<m2::PointD> const & universeCentres, size_t slot)
{
  if (universeCentres.size() == universeAscendingNest.size())
    return universeCentres[slot];
  return MercatorCentreFromNestId(universeAscendingNest[slot]);
}

std::optional<uint32_t> CompactIndexForSlot(SpaFile const & file,
                                            SettlementContainmentIndex const & settlements,
                                            uint32_t sentinel, size_t areaCount, size_t slot,
                                            std::vector<int64_t> const & universeAscendingNest,
                                            std::vector<m2::PointD> const & universeCentres,
                                            bool includeSentinelSlots)
{
  uint32_t const assign = file.m_assignments[slot];
  if (assign != sentinel)
  {
    if (assign < areaCount && file.m_areas[assign].IsAssignable())
      return assign;
    return std::nullopt;
  }
  if (!includeSentinelSlots)
    return std::nullopt;

  ExplorationArea const * area =
      settlements.Select(CentreForSlot(universeAscendingNest, universeCentres, slot));
  if (area == nullptr)
    return std::nullopt;
  uint32_t const idx = area->m_compactIndex;
  if (idx >= areaCount)
    return std::nullopt;
  return idx;
}

void AccumulateSlot(SpaFile const & file, SettlementContainmentIndex const & settlements,
                    uint32_t sentinel, size_t areaCount, size_t slot,
                    std::vector<int64_t> const & universeAscendingNest,
                    std::vector<m2::PointD> const & universeCentres, std::vector<uint64_t> & counts,
                    bool includeSentinelSlots)
{
  if (auto const idx = CompactIndexForSlot(file, settlements, sentinel, areaCount, slot,
                                           universeAscendingNest, universeCentres,
                                           includeSentinelSlots))
    ++counts[*idx];
}

void AccumulateExplored(SpaFile const & file, SettlementContainmentIndex const & settlements,
                        uint32_t sentinel, size_t areaCount,
                        std::vector<int64_t> const & universeAscendingNest,
                        std::vector<m2::PointD> const & universeCentres,
                        std::vector<int64_t> const & exploredAscendingNest,
                        std::vector<uint64_t> & explored, bool includeSentinelSlots)
{
  size_t universeSlot = 0;
  int64_t prevExplored = std::numeric_limits<int64_t>::min();
  bool sawExplored = false;
  for (int64_t healpix : exploredAscendingNest)
  {
    if (sawExplored)
      CHECK_GREATER(healpix, prevExplored, ());
    sawExplored = true;
    prevExplored = healpix;

    while (universeSlot < universeAscendingNest.size() &&
           universeAscendingNest[universeSlot] < healpix)
    {
      ++universeSlot;
    }
    if (universeSlot >= universeAscendingNest.size() ||
        universeAscendingNest[universeSlot] != healpix)
    {
      continue;
    }
    AccumulateSlot(file, settlements, sentinel, areaCount, universeSlot, universeAscendingNest,
                   universeCentres, explored, includeSentinelSlots);
  }
}
}  // namespace

void AreaCompletionCache::ApplyCounts(SpaFile const & file, std::vector<uint64_t> const & totals,
                                      std::vector<uint64_t> const & explored)
{
  size_t const areaCount = file.m_areas.size();
  m_rows.clear();
  m_rows.reserve(areaCount);
  for (size_t i = 0; i < areaCount; ++i)
  {
    AreaCompletionCounts row;
    row.m_compactIndex = file.m_areas[i].m_compactIndex;
    row.m_osmId = StableOsmId(file.m_areas[i]);
    row.m_total = totals[i];
    row.m_explored = explored[i];
    if (row.m_explored > row.m_total)
      row.m_explored = row.m_total;
    m_rows.push_back(row);
  }
}

void AreaCompletionCache::Invalidate()
{
  m_valid = false;
  m_mapDataVersion = 0;
  m_policyVersion = 0;
  m_universeSize = 0;
  m_exploredCount = 0;
  m_rows.clear();
}

AreaCompletionCache AreaCompletionCache::Build(ExplorationAreaResolver const & resolver,
                                               std::vector<int64_t> const & universeAscendingNest,
                                               std::vector<m2::PointD> const & universeCentres,
                                               std::vector<int64_t> const & exploredAscendingNest,
                                               bool includeSentinelSlots)
{
  AreaCompletionCache cache;
  SpaFile const & file = resolver.GetFile();
  CHECK_EQUAL(universeAscendingNest.size(), resolver.Universe().size(), ());
  CHECK(universeCentres.empty() || universeCentres.size() == universeAscendingNest.size(), ());
  CHECK_EQUAL(universeAscendingNest.size(), file.m_assignments.size(), ());

  size_t const areaCount = file.m_areas.size();
  std::vector<uint64_t> totals(areaCount, 0);
  std::vector<uint64_t> explored(areaCount, 0);

  uint32_t const sentinel = NoSubdivisionSentinel(file.m_header.m_indexWidth);
  SettlementContainmentIndex const & settlements = resolver.Settlements();

  for (size_t slot = 0; slot < universeAscendingNest.size(); ++slot)
  {
    AccumulateSlot(file, settlements, sentinel, areaCount, slot, universeAscendingNest, universeCentres,
                   totals, includeSentinelSlots);
  }

  AccumulateExplored(file, settlements, sentinel, areaCount, universeAscendingNest, universeCentres,
                     exploredAscendingNest, explored, includeSentinelSlots);

  cache.ApplyCounts(file, totals, explored);
  cache.m_mapDataVersion = file.m_header.m_mapDataVersion;
  cache.m_policyVersion = file.m_header.m_policyVersion;
  cache.m_universeSize = universeAscendingNest.size();
  cache.m_exploredCount = exploredAscendingNest.size();
  cache.m_valid = true;
  return cache;
}

bool AreaCompletionCache::MatchesFingerprint(int64_t mapDataVersion, uint32_t policyVersion,
                                             uint64_t universeSize, uint64_t exploredCount) const
{
  return m_valid && m_mapDataVersion == mapDataVersion && m_policyVersion == policyVersion &&
         m_universeSize == universeSize && m_exploredCount == exploredCount;
}

bool AreaCompletionCache::RecountExplored(ExplorationAreaResolver const & resolver,
                                          std::vector<int64_t> const & universeAscendingNest,
                                          std::vector<m2::PointD> const & universeCentres,
                                          std::vector<int64_t> const & exploredAscendingNest,
                                          bool includeSentinelSlots)
{
  if (!m_valid)
    return false;
  SpaFile const & file = resolver.GetFile();
  CHECK_EQUAL(universeAscendingNest.size(), resolver.Universe().size(), ());
  CHECK(universeCentres.empty() || universeCentres.size() == universeAscendingNest.size(), ());
  CHECK_EQUAL(universeAscendingNest.size(), file.m_assignments.size(), ());
  if (m_rows.size() != file.m_areas.size())
    return false;

  size_t const areaCount = file.m_areas.size();
  std::vector<uint64_t> totals(areaCount, 0);
  std::vector<uint64_t> explored(areaCount, 0);
  for (size_t i = 0; i < areaCount; ++i)
    totals[i] = m_rows[i].m_total;

  uint32_t const sentinel = NoSubdivisionSentinel(file.m_header.m_indexWidth);
  AccumulateExplored(file, resolver.Settlements(), sentinel, areaCount, universeAscendingNest,
                     universeCentres, exploredAscendingNest, explored, includeSentinelSlots);

  ApplyCounts(file, totals, explored);
  m_universeSize = universeAscendingNest.size();
  m_exploredCount = exploredAscendingNest.size();
  m_mapDataVersion = file.m_header.m_mapDataVersion;
  m_policyVersion = file.m_header.m_policyVersion;
  m_valid = true;
  return true;
}

bool AreaCompletionCache::Save(std::string const & path) const
{
  if (!m_valid)
    return false;
  return base::WriteToTempAndRenameToFile(path, [&](std::string const & tmpPath)
  {
    try
    {
      FileWriter writer(tmpPath, FileWriter::OP_WRITE_TRUNCATE);
      WriteToSink(writer, kAccMagic);
      WriteToSink(writer, kAccFormatVersion);
      WriteToSink(writer, m_mapDataVersion);
      WriteToSink(writer, m_policyVersion);
      WriteToSink(writer, m_universeSize);
      WriteToSink(writer, m_exploredCount);
      WriteToSink(writer, static_cast<uint32_t>(m_rows.size()));
      for (auto const & row : m_rows)
      {
        WriteToSink(writer, row.m_compactIndex);
        WriteToSink(writer, row.m_osmId);
        WriteToSink(writer, row.m_explored);
        WriteToSink(writer, row.m_total);
      }
      writer.Flush();
      return true;
    }
    catch (Writer::Exception const & ex)
    {
      LOG(LERROR, ("Failed to write area completion cache", path, ex.what()));
      return false;
    }
  });
}

std::string AreaCompletionCachePath(std::string const & directory, std::string const & countryId)
{
  return base::JoinPath(directory, countryId + ACC_FILE_EXTENSION);
}

AccLoadResult TryLoadAreaCompletionCache(std::string const & path)
{
  AccLoadResult result;
  if (!Platform::IsFileExistsByFullPath(path))
  {
    result.m_status = AccLoadStatus::Missing;
    return result;
  }
  try
  {
    FileReader reader(path);
    ReaderSource<FileReader> src(reader);
    auto const magic = ReadPrimitiveFromSource<uint32_t>(src);
    auto const formatVersion = ReadPrimitiveFromSource<uint32_t>(src);
    if (magic != kAccMagic || formatVersion != kAccFormatVersion)
    {
      result.m_status = AccLoadStatus::Corrupt;
      return result;
    }
    AreaCompletionCache cache;
    cache.m_mapDataVersion = ReadPrimitiveFromSource<int64_t>(src);
    cache.m_policyVersion = ReadPrimitiveFromSource<uint32_t>(src);
    cache.m_universeSize = ReadPrimitiveFromSource<uint64_t>(src);
    cache.m_exploredCount = ReadPrimitiveFromSource<uint64_t>(src);
    uint32_t const rowCount = ReadPrimitiveFromSource<uint32_t>(src);
    cache.m_rows.reserve(rowCount);
    for (uint32_t i = 0; i < rowCount; ++i)
    {
      AreaCompletionCounts row;
      row.m_compactIndex = ReadPrimitiveFromSource<uint32_t>(src);
      row.m_osmId = ReadPrimitiveFromSource<uint64_t>(src);
      row.m_explored = ReadPrimitiveFromSource<uint64_t>(src);
      row.m_total = ReadPrimitiveFromSource<uint64_t>(src);
      cache.m_rows.push_back(row);
    }
    if (src.Size() != 0)
    {
      result.m_status = AccLoadStatus::Corrupt;
      return result;
    }
    cache.m_valid = true;
    result.m_cache = std::move(cache);
    result.m_status = AccLoadStatus::Ok;
  }
  catch (...)
  {
    result.m_status = AccLoadStatus::Corrupt;
    result.m_cache = {};
  }
  return result;
}

std::optional<AreaCompletionCounts> AreaCompletionCache::Get(uint32_t compactIndex) const
{
  if (!m_valid)
    return std::nullopt;
  for (auto const & row : m_rows)
  {
    if (row.m_compactIndex == compactIndex)
      return row;
  }
  return std::nullopt;
}

std::optional<AreaCompletionCounts> AreaCompletionCache::GetByOsmId(uint64_t osmId) const
{
  if (!m_valid || osmId == 0)
    return std::nullopt;
  for (auto const & row : m_rows)
  {
    if (row.m_osmId == osmId)
      return row;
  }
  return std::nullopt;
}

double AreaCompletionCache::GetFraction(uint32_t compactIndex) const
{
  auto const counts = Get(compactIndex);
  if (!counts)
    return 0.0;
  return AreaCompletionFraction(*counts);
}

bool AreaCompletionCache::AddExploredHealpix(ExplorationAreaResolver const & resolver,
                                             int64_t healpixNestId)
{
  if (!m_valid)
    return false;

  auto const & universe = resolver.Universe();
  auto const it = std::lower_bound(universe.begin(), universe.end(), healpixNestId);
  if (it == universe.end() || *it != healpixNestId)
    return false;

  SpaFile const & file = resolver.GetFile();
  size_t const slot = static_cast<size_t>(it - universe.begin());
  uint32_t const sentinel = NoSubdivisionSentinel(file.m_header.m_indexWidth);
  auto const compact = CompactIndexForSlot(file, resolver.Settlements(), sentinel, file.m_areas.size(),
                                           slot, universe, {}, true /* includeSentinelSlots */);
  if (!compact)
    return false;

  for (auto & row : m_rows)
  {
    if (row.m_compactIndex != *compact)
      continue;
    if (row.m_explored >= row.m_total)
      return false;
    ++row.m_explored;
    return true;
  }
  return false;
}
}  // namespace street_pixels
