#include "testing/testing.hpp"

#include "map/area_milestone_presentation.hpp"
#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"

#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "street_pixels_areas/area_milestone_store.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/completion_card.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"

#include "street_pixels_config/country_config.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string CcAmPath(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void CcAmRemove(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
  Platform::RemoveFileIfExists(path + "-wal");
  Platform::RemoveFileIfExists(path + "-shm");
}

std::vector<m2::PointD> CcAmLonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput CcAmMakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
                                                std::vector<m2::PointD> const & ring)
{
  street_pixels::AreaCandidateInput input;
  input.m_osmId = osmId;
  input.m_osmType = street_pixels::OsmObjectType::Relation;
  input.m_geometrySource = street_pixels::GeometrySource::TrueClosedRing;
  input.m_name = name;
  input.m_kind = "admin";
  input.m_adminLevel = adminLevel;
  input.m_lonLatRings = {ring};
  return input;
}

struct CcAmFixture
{
  std::string leaf;
  std::string spaPath;
  std::string pixPath;
  std::string dbPath;
  int64_t mapDataVersion = 42;
  std::vector<m2::PointD> samples;
};

CcAmFixture MakeCcAmFixture(std::string const & leaf)
{
  CcAmFixture fx;
  fx.leaf = leaf;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.pixPath = CcAmPath(leaf + ".pix");
  fx.dbPath = CcAmPath(leaf + "_milestones.db");
  CcAmRemove(fx.spaPath);
  CcAmRemove(fx.pixPath);
  CcAmRemove(fx.dbPath);

  auto const config = street_pixels::CountryConfig::LoadFromString(R"({
  "policy_version": 1,
  "schema_version": 1,
  "countries": {
    "FI": {
      "mwm_root_ids": ["Finland"],
      "subdivision_admin_levels": [10, 9, 11],
      "settlement_admin_levels": [8],
      "place_boundaries": { "enabled": true, "place_types": ["neighbourhood"] }
    }
  }
})");
  auto const policy = config.GetByIso("FI");

  std::vector<street_pixels::ExplorationArea> areas;
  for (auto const & input : {CcAmMakeAdmin(10, 10, "District", CcAmLonLatBox(24.2, 60.2, 24.8, 60.8)),
                             CcAmMakeAdmin(8, 8, "City", CcAmLonLatBox(24.0, 60.0, 25.0, 61.0))})
  {
    auto result = street_pixels::FilterExplorationCandidate(input, policy);
    TEST(result.m_area.has_value(), ());
    areas.push_back(*result.m_area);
  }

  int64_t const districtId = street_pixels_tests::PixelIdForLatLon(60.5, 24.5);
  int64_t const cityOnlyId = street_pixels_tests::PixelIdForLatLon(60.1, 24.1);
  int64_t const outsideId = street_pixels_tests::PixelIdForLatLon(70.0, 30.0);

  std::vector<std::pair<int64_t, m2::PointD>> universeRows = {
      {districtId, mercator::FromLatLon(60.5, 24.5)},
      {cityOnlyId, mercator::FromLatLon(60.1, 24.1)},
      {outsideId, mercator::FromLatLon(70.0, 30.0)},
  };
  std::sort(universeRows.begin(), universeRows.end(),
            [](auto const & a, auto const & b) { return a.first < b.first; });
  std::vector<int64_t> universeIds;
  fx.samples.clear();
  for (auto const & row : universeRows)
  {
    universeIds.push_back(row.first);
    fx.samples.push_back(row.second);
  }

  street_pixels::SpaWriteParams params;
  params.m_mapDataVersion = fx.mapDataVersion;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = leaf;
  street_pixels::WriteExplorationSidecar(fx.spaPath, areas, fx.samples, policy, params);

  street_pixels_file::ExploredEverLiveMap seed{{districtId, true}};
  TEST(street_pixels_file::SaveRematchedUniverse(
           fx.pixPath, std::set<int64_t>(universeIds.begin(), universeIds.end()), seed, fx.mapDataVersion),
       ());
  return fx;
}

void CleanupCcAm(CcAmFixture const & fx)
{
  CcAmRemove(fx.spaPath);
  CcAmRemove(fx.pixPath);
  CcAmRemove(fx.dbPath);
  street_pixels::AreaMilestoneStore::Instance().Reopen(street_pixels::AreaMilestoneStore::DefaultDbPath());
}

street_pixels::CompletionCardSource MakeRectSource()
{
  street_pixels::CompletionCardSource source;
  source.m_displayName = "District";
  source.m_rings = {{{0, 0}, {10, 0}, {10, 5}, {0, 5}}};
  source.m_completed100At = 1700000000;
  return source;
}

void TestRingsEqual(std::vector<std::vector<m2::PointD>> const & lhs, std::vector<std::vector<m2::PointD>> const & rhs)
{
  TEST_EQUAL(lhs.size(), rhs.size(), ());
  for (size_t i = 0; i < lhs.size(); ++i)
  {
    TEST_EQUAL(lhs[i].size(), rhs[i].size(), ());
    for (size_t j = 0; j < lhs[i].size(); ++j)
      TEST_EQUAL(lhs[i][j], rhs[i][j], ());
  }
}

bool HasInkNear(std::vector<uint8_t> const & rgba, uint32_t width, uint32_t height, int cx, int cy, int radius)
{
  int const w = static_cast<int>(width);
  int const h = static_cast<int>(height);
  for (int y = cy - radius; y <= cy + radius; ++y)
  {
    for (int x = cx - radius; x <= cx + radius; ++x)
    {
      if (x < 0 || y < 0 || x >= w || y >= h)
        continue;
      size_t const i = (static_cast<size_t>(y) * width + static_cast<size_t>(x)) * 4;
      if (rgba[i] < 80)
        return true;
    }
  }
  return false;
}

bool IsBackgroundPixel(std::vector<uint8_t> const & rgba, uint32_t width, int x, int y)
{
  size_t const i = (static_cast<size_t>(y) * width + static_cast<size_t>(x)) * 4;
  return rgba[i] > 200;
}

bool LabelContains(std::string const & label, std::string const & token)
{
  return label.find(token) != std::string::npos;
}
}  // namespace

UNIT_TEST(CompletionCard_DenyListFieldsAbsent)
{
  uint64_t const osmId = 999;
  uint32_t const compactIndex = 7;
  auto source = MakeRectSource();
  TEST_NOT_EQUAL(source.m_displayName, std::to_string(osmId), ());
  TEST_NOT_EQUAL(source.m_displayName, std::to_string(compactIndex), ());
  auto const model = street_pixels::ComposeCompletionCard(source);
  TEST(model.has_value(), ());

  auto const present = street_pixels::PresentFieldNames(*model);
  auto const permitted = street_pixels::CompletionCardPermittedKeys();
  for (auto const & key : present)
  {
    TEST(std::find(permitted.begin(), permitted.end(), key) != permitted.end(), (key));
  }
  auto const label = street_pixels::CompletionCardLabelText(*model);
  auto const debug = DebugPrint(*model);
  for (auto const & denied : street_pixels::CompletionCardDeniedKeys())
  {
    TEST(std::find(present.begin(), present.end(), denied) == present.end(), (denied));
    TEST(!LabelContains(label, denied), (denied, label));
    TEST(!LabelContains(debug, denied), (denied, debug));
  }
  TEST(!LabelContains(label, "999"), (label));
  TEST(!LabelContains(label, "7"), (label));
  TEST(!LabelContains(debug, "999"), (debug));
}

UNIT_TEST(CompletionCard_ComposeWithoutNicknameOrDate)
{
  auto source = MakeRectSource();
  auto const model = street_pixels::ComposeCompletionCard(source);
  TEST(model.has_value(), ());
  TEST(!model->m_nickname.has_value(), ());
  TEST(!model->m_completedDate.has_value(), ());
  TEST_EQUAL(model->m_areaDisplayName, "District", ());
  TEST_EQUAL(model->m_headline, street_pixels::kCompletionCardHeadline, ());
  TEST_EQUAL(model->m_branding, street_pixels::kCompletionCardBranding, ());
  TEST(model->m_competitionLine.empty(), ());
}

UNIT_TEST(CompletionCard_RingsOnlyGeometryMatchesOutline)
{
  auto source = MakeRectSource();
  std::vector<m2::PointD> const gps = {{1, 1}, {2, 2}, {3, 1}};
  auto const model = street_pixels::ComposeCompletionCard(source);
  TEST(model.has_value(), ());
  TestRingsEqual(model->m_outlineRings, source.m_rings);
  TEST(model->m_outlineRings.size() != 1 || model->m_outlineRings.front() != gps, ());

  auto const projected =
      street_pixels::ProjectOutlineToPixels(model->m_outlineRings, street_pixels::kCompletionCardOutlineSize,
                                            street_pixels::kCompletionCardOutlineSize);
  TEST_EQUAL(projected.size(), 1u, ());
  TEST_EQUAL(projected.front().size(), 4u, ());
  m2::RectD box;
  for (auto const & p : projected.front())
    box.Add(p);
  TEST(box.SizeX() > 1.0, (box.SizeX()));
  TEST(box.SizeY() > 1.0, (box.SizeY()));
  TEST(std::abs(box.SizeX() - box.SizeY()) > 1.0, (box.SizeX(), box.SizeY()));

  std::vector<uint8_t> rgba;
  TEST(street_pixels::RasteriseCompletionCard(*model, street_pixels::kCompletionCardOutlineSize,
                                              street_pixels::kCompletionCardOutlineSize, rgba),
       ());
  auto const & ring = projected.front();
  for (size_t i = 0; i < ring.size(); ++i)
  {
    m2::PointD const mid((ring[i].x + ring[(i + 1) % ring.size()].x) * 0.5,
                         (ring[i].y + ring[(i + 1) % ring.size()].y) * 0.5);
    TEST(HasInkNear(rgba, street_pixels::kCompletionCardOutlineSize, street_pixels::kCompletionCardOutlineSize,
                    static_cast<int>(std::lround(mid.x)), static_cast<int>(std::lround(mid.y)), 6),
         (i, mid.x, mid.y));
  }
  m2::PointD centre;
  for (auto const & p : ring)
  {
    centre.x += p.x;
    centre.y += p.y;
  }
  centre.x /= static_cast<double>(ring.size());
  centre.y /= static_cast<double>(ring.size());
  TEST(IsBackgroundPixel(rgba, street_pixels::kCompletionCardOutlineSize, static_cast<int>(std::lround(centre.x)),
                         static_cast<int>(std::lround(centre.y))),
       (centre.x, centre.y));
}

UNIT_TEST(CompletionCard_IncludeDateOnlyWhenRequested)
{
  auto source = MakeRectSource();
  auto const omitted = street_pixels::ComposeCompletionCard(source);
  TEST(omitted.has_value(), ());
  TEST(!omitted->m_completedDate.has_value(), ());

  street_pixels::CompletionCardOptions withDate;
  withDate.includeDate = true;
  auto const dated = street_pixels::ComposeCompletionCard(source, withDate);
  TEST(dated.has_value(), ());
  TEST(dated->m_completedDate.has_value(), ());
  TEST_EQUAL(dated->m_completedDate->size(), 10u, ());
  TEST_EQUAL(std::count(dated->m_completedDate->begin(), dated->m_completedDate->end(), '-'), 2, ());
  TEST(dated->m_completedDate->find('T') == std::string::npos, ());
  TEST(dated->m_completedDate->find(':') == std::string::npos, ());

  source.m_completed100At.reset();
  auto const missing = street_pixels::ComposeCompletionCard(source, withDate);
  TEST(missing.has_value(), ());
  TEST(!missing->m_completedDate.has_value(), ());
}

UNIT_TEST(CompletionCard_NicknameOmittedWhenEmpty)
{
  auto source = MakeRectSource();
  street_pixels::CompletionCardOptions options;
  options.nickname = std::string();
  auto empty = street_pixels::ComposeCompletionCard(source, options);
  TEST(empty.has_value(), ());
  TEST(!empty->m_nickname.has_value(), ());

  options.nickname = std::string("   ");
  auto ws = street_pixels::ComposeCompletionCard(source, options);
  TEST(ws.has_value(), ());
  TEST(!ws->m_nickname.has_value(), ());

  options.nickname = std::string("Ada");
  auto named = street_pixels::ComposeCompletionCard(source, options);
  TEST(named.has_value(), ());
  TEST(named->m_nickname.has_value(), ());
  TEST_EQUAL(*named->m_nickname, "Ada", ());
}

UNIT_TEST(CompletionCard_DisplayNameNeverMwmId)
{
  auto source = MakeRectSource();
  auto const model = street_pixels::ComposeCompletionCard(source);
  TEST(model.has_value(), ());
  TEST_EQUAL(model->m_areaDisplayName, "District", ());
  auto const label = street_pixels::CompletionCardLabelText(*model);
  TEST(!LabelContains(label, "Finland_FakeLeaf"), (label));
}

UNIT_TEST(CompletionCard_CompetitionLineStubEmpty)
{
  auto source = MakeRectSource();
  auto const model = street_pixels::ComposeCompletionCard(source);
  TEST(model.has_value(), ());
  TEST(model->m_competitionLine.empty(), ());
  auto const label = street_pixels::CompletionCardLabelText(*model);
  TEST(!LabelContains(label, "invalid"), (label));
  TEST(!LabelContains(model->m_headline, "invalid"), ());
}

UNIT_TEST(CompletionCard_EmptyRingsNoCard)
{
  street_pixels::CompletionCardSource source;
  source.m_displayName = "District";
  auto const model = street_pixels::ComposeCompletionCard(source);
  TEST(!model.has_value(), ());
}

UNIT_TEST(CompletionCard_TransientFileOverwritesAndDeletes)
{
  auto source = MakeRectSource();
  auto const model = street_pixels::ComposeCompletionCard(source);
  TEST(model.has_value(), ());
  street_pixels::DeleteCompletionCardTransient();
  std::string const path = street_pixels::CompletionCardTransientPath();
  TEST_EQUAL(path, GetPlatform().TmpPathForFile(street_pixels::kCompletionCardTransientFile), ());
  TEST(path.find(GetPlatform().TmpDir()) == 0, (path, GetPlatform().TmpDir()));
  TEST(street_pixels::WriteCompletionCardTransient(*model), ());
  TEST(Platform::IsFileExistsByFullPath(path), (path));
  TEST(street_pixels::WriteCompletionCardTransient(*model), ());
  TEST_EQUAL(street_pixels::CompletionCardTransientPath(), path, ());
  TEST(Platform::IsFileExistsByFullPath(path), (path));
  street_pixels::DeleteCompletionCardTransient();
  TEST(!Platform::IsFileExistsByFullPath(path), (path));
}

UNIT_TEST(CompletionCard_ManagerBindsFromHundredPercentPeek)
{
  auto fx = MakeCcAmFixture("sp067_cc_bind");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  auto peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P100, ());

  auto loaded = street_pixels::TryLoadExplorationSidecar(fx.spaPath);
  TEST_EQUAL(loaded.m_status, street_pixels::SpaLoadStatus::Ok, ());
  auto const * area = street_pixels::FindAreaByCompactIndex(loaded.m_file, peek->m_compactIndex);
  TEST(area != nullptr, ());

  auto card = manager.GetCompletionCardForCurrentPresentation();
  TEST(card.has_value(), ());
  TestRingsEqual(card->m_outlineRings, area->m_rings);
  TEST(card->m_outlineRings.size() != 1 || card->m_outlineRings.front() != fx.samples, ());
  TEST(!card->m_completedDate.has_value(), ());
  auto rec = manager.GetAreaMilestoneRecord(peek->m_osmId);
  TEST(rec.has_value(), ());
  TEST(rec->m_completed100At.has_value(), ());

  manager.AcknowledgeAreaMilestonePresentation();
  peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P50, ());
  TEST(!manager.GetCompletionCardForCurrentPresentation().has_value(), ());

  manager.AcknowledgeAreaMilestonePresentation();
  peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P25, ());
  TEST(!manager.GetCompletionCardForCurrentPresentation().has_value(), ());

  CleanupCcAm(fx);
}
