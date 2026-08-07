#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"

#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/focused_area_progress.hpp"
#include "street_pixels_areas/sparse_assignment_store.hpp"

#include "street_pixels_config/country_config.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "geometry/mercator.hpp"

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string PathJoin(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void RemovePath(std::string const & path) { Platform::RemoveFileIfExists(path); }

std::vector<m2::PointD> LonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput MakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
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

struct DemoFixture
{
  std::string leaf = "sp035_desktop_demo";
  std::string spaPath;
  std::string pixPath;
  int64_t mapDataVersion = 42;
  int64_t districtId = 0;
  int64_t cityOnlyId = 0;
  int64_t outsideId = 0;
  m2::PointD districtCentre;
  m2::PointD cityOnlyCentre;
  m2::PointD outsideCentre;
};

DemoFixture BuildFixture()
{
  DemoFixture fx;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), fx.leaf);
  fx.pixPath = PathJoin(fx.leaf + ".pix");
  RemovePath(fx.spaPath);
  RemovePath(fx.pixPath);
  RemovePath(street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), fx.leaf));

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
  for (auto const & input : {MakeAdmin(10, 10, "District", LonLatBox(24.2, 60.2, 24.8, 60.8)),
                             MakeAdmin(8, 8, "City", LonLatBox(24.0, 60.0, 25.0, 61.0))})
  {
    auto result = street_pixels::FilterExplorationCandidate(input, policy);
    CHECK(result.m_area.has_value(), ());
    areas.push_back(*result.m_area);
  }

  fx.districtId = street_pixels_tests::PixelIdForLatLon(60.5, 24.5);
  fx.cityOnlyId = street_pixels_tests::PixelIdForLatLon(60.1, 24.1);
  fx.outsideId = street_pixels_tests::PixelIdForLatLon(70.0, 30.0);
  fx.districtCentre = mercator::FromLatLon(60.5, 24.5);
  fx.cityOnlyCentre = mercator::FromLatLon(60.1, 24.1);
  fx.outsideCentre = mercator::FromLatLon(70.0, 30.0);

  std::vector<std::pair<int64_t, m2::PointD>> rows = {
      {fx.districtId, fx.districtCentre},
      {fx.cityOnlyId, fx.cityOnlyCentre},
      {fx.outsideId, fx.outsideCentre},
  };
  std::sort(rows.begin(), rows.end(), [](auto const & a, auto const & b) { return a.first < b.first; });
  std::vector<int64_t> universeIds;
  std::vector<m2::PointD> samples;
  for (auto const & row : rows)
  {
    universeIds.push_back(row.first);
    samples.push_back(row.second);
  }

  street_pixels::SpaWriteParams params;
  params.m_mapDataVersion = fx.mapDataVersion;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = fx.leaf;
  street_pixels::WriteExplorationSidecar(fx.spaPath, areas, samples, policy, params);

  street_pixels_file::ExploredEverLiveMap seed{{fx.districtId, true}};
  CHECK(street_pixels_file::SaveRematchedUniverse(
            fx.pixPath, std::set<int64_t>(universeIds.begin(), universeIds.end()), seed, fx.mapDataVersion),
        ());
  return fx;
}

QString FormatBadge(street_pixels::FocusedAreaProgress const & progress)
{
  if (!progress.m_hasFocus || progress.m_displayName.empty())
    return QStringLiteral("(hidden — no focused area)");
  if (!progress.m_fractionValid)
    return QString::fromStdString(progress.m_displayName);
  double const percent = std::round(progress.m_fraction * 1000.0) / 10.0;
  return QStringLiteral("%1 • %2%")
      .arg(QString::fromStdString(progress.m_displayName))
      .arg(percent, 0, 'f', 1);
}
}  // namespace

class BadgeDemoWindow : public QWidget
{
public:
  BadgeDemoWindow()
  {
    setWindowTitle(QStringLiteral("SP-035 Focused Area Badge Desktop Demo"));
    resize(640, 480);

    m_badge = new QLabel(this);
    m_badge->setObjectName(QStringLiteral("explorationBadge"));
    m_badge->setAlignment(Qt::AlignCenter);
    m_badge->setStyleSheet(
        QStringLiteral("QLabel#explorationBadge { font-size: 22px; font-weight: 600; padding: 16px; "
                       "background: #1b4332; color: #d8f3dc; border-radius: 8px; }"));

    m_log = new QTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setObjectName(QStringLiteral("demoLog"));

    auto * focusDistrict = new QPushButton(QStringLiteral("Focus District (100%)"), this);
    focusDistrict->setObjectName(QStringLiteral("focusDistrict"));
    auto * focusCity = new QPushButton(QStringLiteral("Focus City (0%)"), this);
    focusCity->setObjectName(QStringLiteral("focusCity"));
    auto * focusPoint = new QPushButton(QStringLiteral("TryFocusAtPoint district centre"), this);
    focusPoint->setObjectName(QStringLiteral("focusPoint"));
    auto * clearFocus = new QPushButton(QStringLiteral("Clear focus"), this);
    clearFocus->setObjectName(QStringLiteral("clearFocus"));
    auto * invalidate = new QPushButton(QStringLiteral("Invalidate cache (name only)"), this);
    invalidate->setObjectName(QStringLiteral("invalidateCache"));
    auto * rebuild = new QPushButton(QStringLiteral("Rebuild cache"), this);
    rebuild->setObjectName(QStringLiteral("rebuildCache"));
    auto * runScript = new QPushButton(QStringLiteral("Run full scripted pass"), this);
    runScript->setObjectName(QStringLiteral("runScript"));

    auto * layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("Primary badge (Android MapButtonsController format):"), this));
    layout->addWidget(m_badge);
    layout->addWidget(focusDistrict);
    layout->addWidget(focusCity);
    layout->addWidget(focusPoint);
    layout->addWidget(clearFocus);
    layout->addWidget(invalidate);
    layout->addWidget(rebuild);
    layout->addWidget(runScript);
    layout->addWidget(m_log);

    m_fixture = BuildFixture();
    m_dataSource = std::make_unique<FrozenDataSource>();
    m_manager = std::make_unique<StreetPixelsManager>(*m_dataSource);
    CHECK(m_manager->RebuildAreaCompletionCache(m_fixture.leaf, m_fixture.spaPath, m_fixture.mapDataVersion), ());
    AppendLog(QStringLiteral("Fixture ready: District explored 1/1, City 0/1, leaf=%1")
                  .arg(QString::fromStdString(m_fixture.leaf)));
    RefreshBadge(QStringLiteral("startup"));

    connect(focusDistrict, &QPushButton::clicked, this, [this]() {
      bool const ok = m_manager->SetFocusedArea(0, m_fixture.spaPath);
      RefreshBadge(QStringLiteral("SetFocusedArea(District)=%1").arg(ok));
    });
    connect(focusCity, &QPushButton::clicked, this, [this]() {
      bool const ok = m_manager->SetFocusedArea(1, m_fixture.spaPath);
      RefreshBadge(QStringLiteral("SetFocusedArea(City)=%1").arg(ok));
    });
    connect(focusPoint, &QPushButton::clicked, this, [this]() {
      bool const ok =
          m_manager->TryFocusAtPoint(m_fixture.districtCentre, m_fixture.spaPath, m_fixture.mapDataVersion);
      if (!ok)
      {
        // ResourcesDir policy may be missing in this harness; fall back like unit test.
        m_manager->SetFocusedArea(0, m_fixture.spaPath);
        RefreshBadge(QStringLiteral("TryFocusAtPoint failed; SetFocusedArea fallback"));
      }
      else
      {
        RefreshBadge(QStringLiteral("TryFocusAtPoint(district)=true"));
      }
    });
    connect(clearFocus, &QPushButton::clicked, this, [this]() {
      m_manager->ClearFocusedArea();
      RefreshBadge(QStringLiteral("ClearFocusedArea"));
    });
    connect(invalidate, &QPushButton::clicked, this, [this]() {
      m_manager->InvalidateAreaCompletionCache();
      RefreshBadge(QStringLiteral("InvalidateAreaCompletionCache"));
    });
    connect(rebuild, &QPushButton::clicked, this, [this]() {
      bool const ok =
          m_manager->RebuildAreaCompletionCache(m_fixture.leaf, m_fixture.spaPath, m_fixture.mapDataVersion);
      RefreshBadge(QStringLiteral("RebuildAreaCompletionCache=%1").arg(ok));
    });
    connect(runScript, &QPushButton::clicked, this, [this]() { RunScriptedPass(); });
  }

  ~BadgeDemoWindow() override
  {
    RemovePath(m_fixture.spaPath);
    RemovePath(m_fixture.pixPath);
    RemovePath(street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), m_fixture.leaf));
  }

private:
  void AppendLog(QString const & line)
  {
    m_log->append(line);
    LOG(LINFO, (line.toStdString()));
  }

  void RefreshBadge(QString const & reason)
  {
    auto const progress = m_manager->GetFocusedAreaProgress();
    QString const text = FormatBadge(progress);
    m_badge->setText(text);
    AppendLog(QStringLiteral("[%1] badge=\"%2\" hasFocus=%3 fractionValid=%4 name=\"%5\" fraction=%6 neverMwmId=%7")
                  .arg(reason)
                  .arg(text)
                  .arg(progress.m_hasFocus)
                  .arg(progress.m_fractionValid)
                  .arg(QString::fromStdString(progress.m_displayName))
                  .arg(progress.m_fraction)
                  .arg(progress.m_displayName != m_fixture.leaf));
  }

  void RunScriptedPass()
  {
    AppendLog(QStringLiteral("=== scripted pass begin ==="));
    m_manager->ClearFocusedArea();
    RefreshBadge(QStringLiteral("script-clear"));

    CHECK(m_manager->SetFocusedArea(0, m_fixture.spaPath), ());
    auto p = m_manager->GetFocusedAreaProgress();
    CHECK(p.m_hasFocus, ());
    CHECK_EQUAL(p.m_displayName, "District", ());
    CHECK(p.m_fractionValid, ());
    CHECK_EQUAL(p.m_fraction, 1.0, ());
    CHECK(p.m_displayName != m_fixture.leaf, ());
    RefreshBadge(QStringLiteral("script-district"));

    CHECK(m_manager->SetFocusedArea(1, m_fixture.spaPath), ());
    p = m_manager->GetFocusedAreaProgress();
    CHECK_EQUAL(p.m_displayName, "City", ());
    CHECK(p.m_fractionValid, ());
    CHECK_EQUAL(p.m_fraction, 0.0, ());
    RefreshBadge(QStringLiteral("script-city"));

    m_manager->InvalidateAreaCompletionCache();
    p = m_manager->GetFocusedAreaProgress();
    CHECK(p.m_hasFocus, ());
    CHECK(!p.m_fractionValid, ());
    CHECK_EQUAL(p.m_displayName, "City", ());
    RefreshBadge(QStringLiteral("script-invalid-cache"));

    CHECK(m_manager->RebuildAreaCompletionCache(m_fixture.leaf, m_fixture.spaPath, m_fixture.mapDataVersion), ());
    p = m_manager->GetFocusedAreaProgress();
    CHECK(p.m_fractionValid, ());
    RefreshBadge(QStringLiteral("script-rebuild"));

    m_manager->ClearFocusedArea();
    p = m_manager->GetFocusedAreaProgress();
    CHECK(!p.m_hasFocus, ());
    RefreshBadge(QStringLiteral("script-end-clear"));
    AppendLog(QStringLiteral("=== scripted pass PASS ==="));
  }

  DemoFixture m_fixture;
  std::unique_ptr<FrozenDataSource> m_dataSource;
  std::unique_ptr<StreetPixelsManager> m_manager;
  QLabel * m_badge = nullptr;
  QTextEdit * m_log = nullptr;
};

int main(int argc, char ** argv)
{
  QApplication app(argc, argv);
  BadgeDemoWindow window;
  window.show();
  return app.exec();
}
