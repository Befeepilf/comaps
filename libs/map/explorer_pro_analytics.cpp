#include "map/explorer_pro_analytics.hpp"
#include "map/explorer_pro.hpp"

#include "platform/settings.hpp"

#include <sstream>

namespace street_pixels
{
namespace
{
void IncrementCounter(std::string_view key)
{
  uint64_t value = 0;
  settings::TryGet(key, value);
  settings::Set(key, value + 1);
}
}  // namespace

void ExplorerProAnalytics::RecordInfoPageViewed()
{
  if (!explorer_pro::IsCapabilityAvailable(explorer_pro::Capability::GpxImport)
      && !explorer_pro::IsCapabilityAvailable(explorer_pro::Capability::GpxExport)
      && !explorer_pro::IsCapabilityAvailable(explorer_pro::Capability::AdvancedTrackManagement))
    return;
  IncrementCounter(kInfoPageViewedKey);
}

void ExplorerProAnalytics::RecordGpxImportUsage()
{
  if (!explorer_pro::IsCapabilityAvailable(explorer_pro::Capability::GpxImport))
    return;
  IncrementCounter(kGpxImportUsageKey);
}

void ExplorerProAnalytics::RecordGpxExportUsage()
{
  if (!explorer_pro::IsCapabilityAvailable(explorer_pro::Capability::GpxExport))
    return;
  IncrementCounter(kGpxExportUsageKey);
}

ExplorerProAnalyticsSnapshot ExplorerProAnalytics::LoadSnapshot()
{
  ExplorerProAnalyticsSnapshot snapshot;
  settings::TryGet(kInfoPageViewedKey, snapshot.m_infoPageViewed);
  settings::TryGet(kGpxImportUsageKey, snapshot.m_gpxImportUsage);
  settings::TryGet(kGpxExportUsageKey, snapshot.m_gpxExportUsage);
  return snapshot;
}

std::array<std::pair<std::string_view, uint64_t>, 3> ExplorerProAnalytics::SerializedSnapshot()
{
  ExplorerProAnalyticsSnapshot const snapshot = LoadSnapshot();
  return {{
      {kInfoPageViewedName, snapshot.m_infoPageViewed},
      {kGpxImportUsageName, snapshot.m_gpxImportUsage},
      {kGpxExportUsageName, snapshot.m_gpxExportUsage},
  }};
}

void ExplorerProAnalytics::ResetForTesting()
{
  settings::Delete(kInfoPageViewedKey);
  settings::Delete(kGpxImportUsageKey);
  settings::Delete(kGpxExportUsageKey);
}

std::string DebugPrint(ExplorerProAnalyticsSnapshot const & snapshot)
{
  std::ostringstream oss;
  oss << "pro-info-viewed=" << snapshot.m_infoPageViewed << " gpx-import-usage=" << snapshot.m_gpxImportUsage
      << " gpx-export-usage=" << snapshot.m_gpxExportUsage;
  return oss.str();
}
}  // namespace street_pixels
