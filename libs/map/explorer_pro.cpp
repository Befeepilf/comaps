#include "map/explorer_pro.hpp"

#include <atomic>

namespace
{
bool g_gpxImportAvailable = false;
bool g_gpxExportAvailable = false;
bool g_advancedTrackManagementAvailable = false;
std::atomic<bool> g_configurationFrozen{false};

explorer_pro::StubEntitlementSource g_stubEntitlementSource;
#ifdef DEBUG
explorer_pro::DebugEntitlementSource g_debugEntitlementSource;
#endif
explorer_pro::EntitlementSource * g_entitlementSource = &g_stubEntitlementSource;

bool & AvailabilityFor(explorer_pro::Capability capability)
{
  switch (capability)
  {
  case explorer_pro::Capability::GpxImport: return g_gpxImportAvailable;
  case explorer_pro::Capability::GpxExport: return g_gpxExportAvailable;
  case explorer_pro::Capability::AdvancedTrackManagement: return g_advancedTrackManagementAvailable;
  }
  return g_gpxImportAvailable;
}
}  // namespace

bool explorer_pro::StubEntitlementSource::IsEntitled() const { return false; }

#ifdef DEBUG
bool explorer_pro::DebugEntitlementSource::IsEntitled() const { return true; }
#endif

void explorer_pro::SetCapabilityAvailable(Capability capability, bool available)
{
  if (g_configurationFrozen)
    return;
  AvailabilityFor(capability) = available;
}

bool explorer_pro::IsCapabilityAvailable(Capability capability) { return AvailabilityFor(capability); }

bool explorer_pro::IsEntitled() { return g_entitlementSource->IsEntitled(); }

void explorer_pro::SetEntitlementSource(EntitlementSource * source)
{
  if (g_configurationFrozen)
    return;
  g_entitlementSource = source != nullptr ? source : &g_stubEntitlementSource;
}

#ifdef DEBUG
void explorer_pro::InstallDebugEntitlementSource()
{
  if (g_configurationFrozen)
    return;
  SetEntitlementSource(&g_debugEntitlementSource);
}
#endif

void explorer_pro::FreezeConfiguration()
{
  g_configurationFrozen = true;
}

void explorer_pro::UnfreezeConfigurationForTesting()
{
  g_configurationFrozen = false;
}

bool explorer_pro::IsCapabilityEnabled(Capability capability)
{
  return IsCapabilityAvailable(capability) && IsEntitled();
}
