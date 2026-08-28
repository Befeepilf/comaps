#include "map/explorer_pro.hpp"

namespace
{
bool g_gpxImportAvailable = false;
bool g_gpxExportAvailable = false;
bool g_advancedTrackManagementAvailable = false;

explorer_pro::StubEntitlementSource g_stubEntitlementSource;
explorer_pro::DebugEntitlementSource g_debugEntitlementSource;
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

bool explorer_pro::DebugEntitlementSource::IsEntitled() const { return true; }

void explorer_pro::SetCapabilityAvailable(Capability capability, bool available)
{
  AvailabilityFor(capability) = available;
}

bool explorer_pro::IsCapabilityAvailable(Capability capability) { return AvailabilityFor(capability); }

bool explorer_pro::IsEntitled() { return g_entitlementSource->IsEntitled(); }

void explorer_pro::SetEntitlementSource(EntitlementSource * source)
{
  g_entitlementSource = source != nullptr ? source : &g_stubEntitlementSource;
}

void explorer_pro::InstallDebugEntitlementSource()
{
  SetEntitlementSource(&g_debugEntitlementSource);
}

bool explorer_pro::IsCapabilityEnabled(Capability capability)
{
  return IsCapabilityAvailable(capability) && IsEntitled();
}
