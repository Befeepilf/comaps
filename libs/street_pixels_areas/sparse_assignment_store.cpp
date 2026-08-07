#include "street_pixels_areas/sparse_assignment_store.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "platform/platform.hpp"

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
bool IsStrictlyAscendingEntries(std::vector<SparseAssignmentEntry> const & entries)
{
  for (size_t i = 1; i < entries.size(); ++i)
  {
    if (entries[i].m_healpixNestId <= entries[i - 1].m_healpixNestId)
      return false;
  }
  return true;
}

bool IsStrictlyAscendingIds(std::vector<int64_t> const & ids)
{
  for (size_t i = 1; i < ids.size(); ++i)
  {
    if (ids[i] <= ids[i - 1])
      return false;
  }
  return true;
}

void WriteSpxHeader(Writer & writer, SpxHeader const & header)
{
  WriteToSink(writer, header.m_magic);
  WriteToSink(writer, header.m_formatVersion);
  WriteToSink(writer, header.m_mapDataVersion);
  WriteToSink(writer, header.m_policyVersion);
  WriteToSink(writer, header.m_entryCount);
  WriteToSink(writer, header.m_indexWidth);
}

SpxHeader ReadSpxHeader(ReaderSource<FileReader> & src)
{
  SpxHeader header;
  header.m_magic = ReadPrimitiveFromSource<uint32_t>(src);
  header.m_formatVersion = ReadPrimitiveFromSource<uint32_t>(src);
  header.m_mapDataVersion = ReadPrimitiveFromSource<int64_t>(src);
  header.m_policyVersion = ReadPrimitiveFromSource<uint32_t>(src);
  header.m_entryCount = ReadPrimitiveFromSource<uint32_t>(src);
  header.m_indexWidth = ReadPrimitiveFromSource<uint8_t>(src);

  if (header.m_magic != kSpxMagic)
    MYTHROW(SpxFormatException, ("Bad .spx magic", header.m_magic));
  if (header.m_formatVersion != kSpxFormatVersion)
    MYTHROW(SpxFormatException, ("Unsupported .spx format_version", header.m_formatVersion));
  if (header.m_indexWidth != 2 && header.m_indexWidth != 4)
    MYTHROW(SpxFormatException, ("Unsupported .spx index_width", header.m_indexWidth));
  return header;
}

SparseAssignmentStore ReadSpxFile(std::string const & path)
{
  FileReader reader(path);
  ReaderSource<FileReader> src(reader);
  SpxHeader header = ReadSpxHeader(src);

  std::vector<SparseAssignmentEntry> entries;
  entries.reserve(header.m_entryCount);
  for (uint32_t i = 0; i < header.m_entryCount; ++i)
  {
    SparseAssignmentEntry entry;
    entry.m_healpixNestId = ReadPrimitiveFromSource<int64_t>(src);
    if (header.m_indexWidth == 2)
      entry.m_compactIndex = ReadPrimitiveFromSource<uint16_t>(src);
    else
      entry.m_compactIndex = ReadPrimitiveFromSource<uint32_t>(src);
    entries.push_back(entry);
  }

  if (src.Size() != 0)
    MYTHROW(SpxFormatException, ("Trailing bytes in .spx", path));
  if (entries.size() != header.m_entryCount)
    MYTHROW(SpxFormatException, ("Entry count mismatch in .spx", path));
  if (!IsStrictlyAscendingEntries(entries))
    MYTHROW(SpxFormatException, ("Non-ascending healpix ids in .spx", path));

  return SparseAssignmentStore(std::move(header), std::move(entries));
}
}  // namespace

std::string SparseAssignmentPath(std::string const & directory, std::string const & countryId)
{
  return base::JoinPath(directory, countryId + SPX_FILE_EXTENSION);
}

std::string SparseAssignmentPathBesidePix(std::string const & pixPath)
{
  return SparseAssignmentPath(base::GetDirectory(pixPath), base::GetNameFromFullPathWithoutExt(pixPath));
}

SparseAssignmentStore::SparseAssignmentStore(SpxHeader header, std::vector<SparseAssignmentEntry> entries)
  : m_header(std::move(header)), m_entries(std::move(entries))
{
  m_header.m_entryCount = static_cast<uint32_t>(m_entries.size());
}

bool SparseAssignmentStore::MatchesVersions(int64_t mapDataVersion, uint32_t policyVersion) const
{
  return m_header.m_mapDataVersion == mapDataVersion && m_header.m_policyVersion == policyVersion;
}

bool SparseAssignmentStore::CoversExplored(std::vector<int64_t> const & exploredAscendingNest) const
{
  if (m_entries.size() != exploredAscendingNest.size())
    return false;
  for (size_t i = 0; i < exploredAscendingNest.size(); ++i)
  {
    if (m_entries[i].m_healpixNestId != exploredAscendingNest[i])
      return false;
  }
  return true;
}

std::optional<uint32_t> SparseAssignmentStore::FindCompactIndex(int64_t healpixNestId) const
{
  auto const it = std::lower_bound(
      m_entries.begin(), m_entries.end(), healpixNestId,
      [](SparseAssignmentEntry const & entry, int64_t id) { return entry.m_healpixNestId < id; });
  if (it == m_entries.end() || it->m_healpixNestId != healpixNestId)
    return std::nullopt;
  return it->m_compactIndex;
}

ExplorationArea const * SparseAssignmentStore::LookupArea(SpaFile const & file, int64_t healpixNestId) const
{
  auto const compact = FindCompactIndex(healpixNestId);
  if (!compact.has_value())
    return nullptr;
  // Prefer store index_width sentinel; also tolerate sidecar sentinel when widths match.
  uint32_t const storeSentinel = NoSubdivisionSentinel(m_header.m_indexWidth);
  if (*compact == storeSentinel)
    return nullptr;
  return FindAreaByCompactIndex(file, *compact);
}

SparseAssignmentStore SparseAssignmentStore::Build(ExplorationAreaResolver const & resolver,
                                                   std::vector<int64_t> const & exploredAscendingNest,
                                                   std::vector<m2::PointD> const & sampleCentres)
{
  if (exploredAscendingNest.size() != sampleCentres.size())
    MYTHROW(SpxFormatException, ("Explored ids and sample centres size mismatch"));
  if (!IsStrictlyAscendingIds(exploredAscendingNest))
    MYTHROW(SpxFormatException, ("Explored healpix ids must be strictly ascending"));

  auto const & spa = resolver.GetFile();
  uint8_t const indexWidth = spa.m_header.m_indexWidth == 0 ? ChooseIndexWidth(spa.m_header.m_areaCount)
                                                            : spa.m_header.m_indexWidth;
  uint32_t const sentinel = NoSubdivisionSentinel(indexWidth);

  std::vector<SparseAssignmentEntry> entries;
  entries.reserve(exploredAscendingNest.size());
  for (size_t i = 0; i < exploredAscendingNest.size(); ++i)
  {
    SparseAssignmentEntry entry;
    entry.m_healpixNestId = exploredAscendingNest[i];
    auto const * area = resolver.LookupByHealpix(entry.m_healpixNestId, sampleCentres[i]);
    if (area == nullptr)
      entry.m_compactIndex = sentinel;
    else
      entry.m_compactIndex = area->m_compactIndex;
    entries.push_back(entry);
  }

  SpxHeader header;
  header.m_magic = kSpxMagic;
  header.m_formatVersion = kSpxFormatVersion;
  header.m_mapDataVersion = spa.m_header.m_mapDataVersion;
  header.m_policyVersion = spa.m_header.m_policyVersion;
  header.m_entryCount = static_cast<uint32_t>(entries.size());
  header.m_indexWidth = indexWidth;
  return SparseAssignmentStore(std::move(header), std::move(entries));
}

SparseAssignmentStore SparseAssignmentStore::Rematerialize(ExplorationAreaResolver const & resolver,
                                                           std::vector<int64_t> const & exploredAscendingNest,
                                                           std::vector<m2::PointD> const & sampleCentres)
{
  return Build(resolver, exploredAscendingNest, sampleCentres);
}

bool SparseAssignmentStore::Save(std::string const & path) const
{
  return base::WriteToTempAndRenameToFile(path, [&](std::string const & tmpPath)
  {
    try
    {
      FileWriter writer(tmpPath, FileWriter::OP_WRITE_TRUNCATE);
      SpxHeader header = m_header;
      header.m_magic = kSpxMagic;
      header.m_formatVersion = kSpxFormatVersion;
      header.m_entryCount = static_cast<uint32_t>(m_entries.size());
      WriteSpxHeader(writer, header);

      uint32_t const sentinel = NoSubdivisionSentinel(header.m_indexWidth);
      for (auto const & entry : m_entries)
      {
        WriteToSink(writer, entry.m_healpixNestId);
        if (header.m_indexWidth == 2)
        {
          if (entry.m_compactIndex != sentinel && entry.m_compactIndex > std::numeric_limits<uint16_t>::max())
            MYTHROW(SpxFormatException, ("Compact index exceeds uint16", entry.m_compactIndex));
          WriteToSink(writer, static_cast<uint16_t>(entry.m_compactIndex));
        }
        else
        {
          WriteToSink(writer, entry.m_compactIndex);
        }
      }
      writer.Flush();
      return true;
    }
    catch (Writer::Exception const & ex)
    {
      LOG(LERROR, ("Failed to write sparse assignment store", path, ex.what()));
      return false;
    }
    catch (SpxFormatException const & ex)
    {
      LOG(LERROR, ("Failed to write sparse assignment store", path, ex.Msg()));
      return false;
    }
  });
}

SpxLoadResult TryLoadSparseAssignmentStore(std::string const & path)
{
  SpxLoadResult result;
  if (!Platform::IsFileExistsByFullPath(path))
  {
    result.m_status = SpxLoadStatus::Missing;
    return result;
  }
  if (Platform::IsDirectory(path))
  {
    LOG(LWARNING, ("Sparse assignment path is a directory", path));
    result.m_status = SpxLoadStatus::Corrupt;
    return result;
  }

  try
  {
    result.m_store = ReadSpxFile(path);
    result.m_status = SpxLoadStatus::Ok;
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Corrupt sparse assignment store", path, ex.Msg()));
    result.m_store = SparseAssignmentStore{};
    result.m_status = SpxLoadStatus::Corrupt;
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("Corrupt sparse assignment store", path, ex.what()));
    result.m_store = SparseAssignmentStore{};
    result.m_status = SpxLoadStatus::Corrupt;
  }
  catch (...)
  {
    LOG(LWARNING, ("Corrupt sparse assignment store", path, "unknown exception"));
    result.m_store = SparseAssignmentStore{};
    result.m_status = SpxLoadStatus::Corrupt;
  }
  return result;
}

SpxLoadResult TryLoadAndVerifySparseAssignmentStore(std::string const & path, int64_t expectedMapDataVersion,
                                                    uint32_t expectedPolicyVersion)
{
  SpxLoadResult result = TryLoadSparseAssignmentStore(path);
  if (result.m_status != SpxLoadStatus::Ok)
    return result;

  if (!result.m_store.MatchesVersions(expectedMapDataVersion, expectedPolicyVersion))
  {
    LOG(LWARNING, ("Sparse assignment version mismatch", path, "map",
                   result.m_store.GetHeader().m_mapDataVersion, expectedMapDataVersion, "policy",
                   result.m_store.GetHeader().m_policyVersion, expectedPolicyVersion));
    result.m_store = SparseAssignmentStore{};
    result.m_status = SpxLoadStatus::VersionMismatch;
  }
  return result;
}

std::optional<SparseAssignmentStore> EnsureSparseAssignmentStore(
    std::string const & spxPath, ExplorationAreaResolver const & resolver,
    std::vector<int64_t> const & exploredAscendingNest, std::vector<m2::PointD> const & sampleCentres)
{
  auto const & spa = resolver.GetFile();
  int64_t const mapDataVersion = spa.m_header.m_mapDataVersion;
  uint32_t const policyVersion = spa.m_header.m_policyVersion;

  auto loaded = TryLoadAndVerifySparseAssignmentStore(spxPath, mapDataVersion, policyVersion);
  // Version match alone is not enough: exploration may have grown since the last
  // save. Rebuild when the sparse set is not exactly the current explored set.
  if (loaded.m_status == SpxLoadStatus::Ok && loaded.m_store.CoversExplored(exploredAscendingNest))
    return std::move(loaded.m_store);

  try
  {
    auto store = SparseAssignmentStore::Rematerialize(resolver, exploredAscendingNest, sampleCentres);
    if (!store.Save(spxPath))
    {
      LOG(LWARNING, ("Failed to save rematerialized sparse assignments", spxPath));
      return std::nullopt;
    }
    return store;
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Failed to rematerialize sparse assignments", spxPath, ex.Msg()));
    return std::nullopt;
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("Failed to rematerialize sparse assignments", spxPath, ex.what()));
    return std::nullopt;
  }
}

std::string DebugPrint(SpxLoadStatus status)
{
  switch (status)
  {
  case SpxLoadStatus::Ok: return "Ok";
  case SpxLoadStatus::Missing: return "Missing";
  case SpxLoadStatus::Corrupt: return "Corrupt";
  case SpxLoadStatus::VersionMismatch: return "VersionMismatch";
  }
  return "UnknownSpxLoadStatus";
}
}  // namespace street_pixels
