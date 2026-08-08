#include "street_pixels_areas/spa_jsonl.hpp"

#include "coding/files_container.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "cppjansson/cppjansson.hpp"

#include "defines.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace street_pixels
{
namespace
{
bool ParseLonLatPoint(json_t * arr, m2::PointD & out)
{
  if (!json_is_array(arr) || json_array_size(arr) < 2)
    return false;
  double lon = 0.0;
  double lat = 0.0;
  try
  {
    FromJSON(json_array_get(arr, 0), lon);
    FromJSON(json_array_get(arr, 1), lat);
  }
  catch (base::Json::Exception const &)
  {
    return false;
  }
  out = {lon, lat};
  return true;
}

bool ParseLonLatRing(json_t * ringArr, std::vector<m2::PointD> & out)
{
  if (!json_is_array(ringArr))
    return false;
  size_t const n = json_array_size(ringArr);
  out.clear();
  out.reserve(n);
  for (size_t i = 0; i < n; ++i)
  {
    m2::PointD pt;
    if (!ParseLonLatPoint(json_array_get(ringArr, i), pt))
      return false;
    out.push_back(pt);
  }
  return out.size() >= 4;
}

bool ReadPolyRing(std::istream & stream, m2::RegionD & poly)
{
  std::string line;
  std::string name;
  if (!std::getline(stream, name))
    return false;
  strings::Trim(name);
  if (name.empty() || name == "END")
    return false;

  poly = {};
  while (stream.good())
  {
    if (!std::getline(stream, line))
      break;
    strings::Trim(line);
    if (line.empty())
      continue;
    if (line == "END")
      break;
    std::istringstream iss(line);
    double lon = 0.0;
    double lat = 0.0;
    iss >> lon >> lat;
    if (iss.fail())
      continue;
    poly.AddPoint(mercator::FromLatLon(lat, lon));
  }
  // Drop inner rings (Osmosis '!' prefix).
  return !name.empty() && name[0] != '!' && poly.IsValid();
}
}  // namespace

std::optional<JsonlRingRecord> ParseJsonlRingLine(std::string const & line)
{
  if (line.empty())
    return std::nullopt;

  base::Json root(line);
  if (!json_is_object(root.get()))
    return std::nullopt;

  JsonlRingRecord rec;
  auto & input = rec.m_input;
  input.m_geometrySource = GeometrySource::TrueClosedRing;

  try
  {
    FromJSONObject(root.get(), "osm_id", input.m_osmId);
    std::string osmType;
    FromJSONObject(root.get(), "osm_type", osmType);
    input.m_osmType = (osmType == "way") ? OsmObjectType::Way : OsmObjectType::Relation;
    FromJSONObjectOptionalField(root.get(), "name", input.m_name);
    FromJSONObject(root.get(), "kind", input.m_kind);
    // Place records may serialize admin_level as JSON null — treat as unset (-1).
    {
      json_t * adminLevel = base::GetJSONOptionalField(root.get(), "admin_level");
      if (adminLevel && !base::JSONIsNull(adminLevel))
        FromJSON(adminLevel, input.m_adminLevel);
      else
        input.m_adminLevel = -1;
    }
    std::string place;
    FromJSONObjectOptionalField(root.get(), "place", place);
    input.m_placeType = place;

    json_t * rings = base::GetJSONObligatoryField(root.get(), "rings");
    if (!json_is_array(rings))
      return std::nullopt;
    size_t const ringCount = json_array_size(rings);
    input.m_lonLatRings.reserve(ringCount);
    for (size_t i = 0; i < ringCount; ++i)
    {
      std::vector<m2::PointD> ring;
      if (!ParseLonLatRing(json_array_get(rings, i), ring))
        return std::nullopt;
      input.m_lonLatRings.push_back(std::move(ring));
    }

    json_t * centroid = base::GetJSONOptionalField(root.get(), "centroid");
    if (centroid)
    {
      m2::PointD c;
      if (ParseLonLatPoint(centroid, c))
        rec.m_centroidLonLat = c;
    }
  }
  catch (base::Json::Exception const &)
  {
    return std::nullopt;
  }

  return rec;
}

bool CentroidInsideAny(m2::PointD const & lonLat, std::vector<m2::RegionD> const & mercatorRegions)
{
  m2::PointD const merc = mercator::FromLatLon(lonLat.y, lonLat.x);
  for (auto const & region : mercatorRegions)
  {
    if (region.Contains(merc))
      return true;
  }
  return false;
}

JsonlFilterStats FilterJsonlRings(std::string const & jsonlPath, CountryPolicy const & policy,
                                  std::vector<m2::RegionD> const * centroidFilterMercator,
                                  std::vector<ExplorationArea> & outAreas)
{
  JsonlFilterStats stats;
  outAreas.clear();

  std::ifstream in(jsonlPath);
  if (!in)
  {
    LOG(LERROR, ("Cannot open JSONL:", jsonlPath));
    return stats;
  }

  std::string line;
  while (std::getline(in, line))
  {
    strings::Trim(line);
    if (line.empty())
      continue;
    auto parsed = ParseJsonlRingLine(line);
    if (!parsed)
    {
      stats.m_rejects["parse_error"] += 1;
      continue;
    }
    ++stats.m_records;

    if (centroidFilterMercator)
    {
      if (!parsed->m_centroidLonLat || !CentroidInsideAny(*parsed->m_centroidLonLat, *centroidFilterMercator))
      {
        stats.m_rejects["outside_leaf"] += 1;
        continue;
      }
    }

    auto result = FilterExplorationCandidate(parsed->m_input, policy);
    if (result.m_reason != RejectReason::Accepted || !result.m_area)
    {
      stats.m_rejects[DebugPrint(result.m_reason)] += 1;
      continue;
    }
    outAreas.push_back(std::move(*result.m_area));
    ++stats.m_admitted;
  }
  return stats;
}

void WriteGeometryOnlyExplorationSidecar(std::string const & path, std::vector<ExplorationArea> areas,
                                         CountryPolicy const & policy, SpaWriteParams const & params)
{
  // Empty sample list → assign_count == 0 (geometry-only fixture / offline emit).
  WriteExplorationSidecar(path, std::move(areas), /*samplePoints=*/{}, policy, params);
}

SpaSectionSizes MeasureSpaSectionSizes(std::string const & path)
{
  SpaSectionSizes sizes;
  FilesContainerR container(path);
  sizes.m_fileBytes = container.GetFileSize();
  auto const hdr = container.GetAbsoluteOffsetAndSize(SPA_HEADER_FILE_TAG);
  auto const areas = container.GetAbsoluteOffsetAndSize(SPA_AREAS_FILE_TAG);
  auto const assign = container.GetAbsoluteOffsetAndSize(SPA_ASSIGN_FILE_TAG);
  sizes.m_hdrBytes = hdr.second;
  sizes.m_areasBytes = areas.second;
  sizes.m_assignBytes = assign.second;
  return sizes;
}

bool LoadPolyFileAsMercatorRegions(std::string const & polyPath, std::vector<m2::RegionD> & outRegions)
{
  outRegions.clear();
  std::ifstream stream(polyPath);
  if (!stream)
    return false;
  std::string title;
  if (!std::getline(stream, title))
    return false;

  m2::RegionD current;
  while (ReadPolyRing(stream, current))
  {
    outRegions.push_back(std::move(current));
    current = {};
  }
  return !outRegions.empty();
}

std::vector<std::pair<uint64_t, std::string>> HelsinkiKnownOsmIds()
{
  // Spot-check only — not an allowlist or country config (SPD-004).
  return {
      {184714, "Kamppi"},
      {184765, "Kallio"},
      {184703, "Punavuori"},
      {184702, "Ullanlinna"},
      {184727, "Etu-Töölö"},
      {184728, "Taka-Töölö"},
      {184655, "Lauttasaari"},
      {184668, "Eira"},
      {184711, "Katajanokka"},
      {184712, "Kruununhaka"},
      {34914, "Helsinki"},
  };
}

std::vector<KnownIdSpotCheck> SpotCheckKnownIds(std::vector<ExplorationArea> const & areas,
                                                std::vector<std::pair<uint64_t, std::string>> const & known)
{
  std::map<uint64_t, ExplorationArea const *> byId;
  for (auto const & area : areas)
    byId[area.m_osmId] = &area;

  std::vector<KnownIdSpotCheck> out;
  out.reserve(known.size());
  for (auto const & [osmId, hint] : known)
  {
    KnownIdSpotCheck row;
    row.m_osmId = osmId;
    row.m_expectedNameHint = hint;
    auto it = byId.find(osmId);
    if (it != byId.end())
    {
      row.m_found = true;
      row.m_actualName = it->second->m_name;
      row.m_nameMatches = (row.m_actualName == row.m_expectedNameHint);
      row.m_role = it->second->m_role;
      row.m_adminLevel = it->second->m_adminLevel;
    }
    out.push_back(std::move(row));
  }
  return out;
}

std::vector<LeafBorder> ListLeafBorders(std::string const & bordersDir, std::string const & namePrefix)
{
  std::vector<LeafBorder> out;
  namespace fs = std::filesystem;
  std::error_code ec;
  if (!fs::is_directory(bordersDir, ec))
    return out;

  for (auto const & entry : fs::directory_iterator(bordersDir, ec))
  {
    if (ec || !entry.is_regular_file())
      continue;
    auto const path = entry.path();
    if (path.extension() != ".poly")
      continue;
    std::string const leafId = path.stem().string();
    if (!namePrefix.empty() && leafId.compare(0, namePrefix.size(), namePrefix) != 0)
      continue;
    LeafBorder leaf;
    leaf.m_leafId = leafId;
    leaf.m_polyPath = path.string();
    out.push_back(std::move(leaf));
  }
  std::sort(out.begin(), out.end(),
            [](LeafBorder const & a, LeafBorder const & b) { return a.m_leafId < b.m_leafId; });
  return out;
}
}  // namespace street_pixels
