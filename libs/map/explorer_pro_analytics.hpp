#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace street_pixels
{
struct ExplorerProAnalyticsSnapshot
{
  uint64_t m_infoPageViewed = 0;
  uint64_t m_gpxImportUsage = 0;
  uint64_t m_gpxExportUsage = 0;
};

class ExplorerProAnalytics
{
public:
  static std::string_view constexpr kInfoPageViewedKey = "Explore.ProInfoViewed";
  static std::string_view constexpr kGpxImportUsageKey = "Explore.GpxImportUsage";
  static std::string_view constexpr kGpxExportUsageKey = "Explore.GpxExportUsage";
  static std::string_view constexpr kInfoPageViewedName = "pro-info-viewed";
  static std::string_view constexpr kGpxImportUsageName = "gpx-import-usage";
  static std::string_view constexpr kGpxExportUsageName = "gpx-export-usage";

  static void RecordInfoPageViewed();
  static void RecordGpxImportUsage();
  static void RecordGpxExportUsage();
  static ExplorerProAnalyticsSnapshot LoadSnapshot();
  static std::array<std::pair<std::string_view, uint64_t>, 3> SerializedSnapshot();
  static void ResetForTesting();
};

std::string DebugPrint(ExplorerProAnalyticsSnapshot const & snapshot);
}  // namespace street_pixels
