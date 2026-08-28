#include "testing/testing.hpp"

#include "map/explorer_pro.hpp"
#include "platform/settings.hpp"

namespace
{
class FakeEntitlementSource : public explorer_pro::EntitlementSource
{
public:
  explicit FakeEntitlementSource(bool entitled) : m_entitled(entitled) {}

  bool IsEntitled() const override { return m_entitled; }

private:
  bool m_entitled;
};

class EntitlementSourceScope
{
public:
  explicit EntitlementSourceScope(explorer_pro::EntitlementSource * source)
  {
    explorer_pro::SetEntitlementSource(source);
  }

  ~EntitlementSourceScope() { explorer_pro::SetEntitlementSource(nullptr); }
};

class CapabilityAvailabilityScope
{
public:
  CapabilityAvailabilityScope(explorer_pro::Capability capability, bool available)
    : m_capability(capability)
    , m_previous(explorer_pro::IsCapabilityAvailable(capability))
  {
    explorer_pro::SetCapabilityAvailable(capability, available);
  }

  ~CapabilityAvailabilityScope() { explorer_pro::SetCapabilityAvailable(m_capability, m_previous); }

private:
  explorer_pro::Capability m_capability;
  bool m_previous;
};

class RestoreStubOnExit
{
public:
  ~RestoreStubOnExit() { explorer_pro::SetEntitlementSource(nullptr); }
};

class ExplorerProUnfreezeOnExit
{
public:
  ~ExplorerProUnfreezeOnExit() { explorer_pro::UnfreezeConfigurationForTesting(); }
};

void ResetCapabilities()
{
  explorer_pro::UnfreezeConfigurationForTesting();
  explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxImport, false);
  explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxExport, false);
  explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::AdvancedTrackManagement, false);
}
}  // namespace

UNIT_TEST(ExplorerPro_DefaultCapabilitiesUnavailable)
{
  ResetCapabilities();
  TEST(!explorer_pro::IsCapabilityAvailable(explorer_pro::Capability::GpxImport), ());
  TEST(!explorer_pro::IsCapabilityAvailable(explorer_pro::Capability::GpxExport), ());
  TEST(!explorer_pro::IsCapabilityAvailable(explorer_pro::Capability::AdvancedTrackManagement), ());
}

UNIT_TEST(ExplorerPro_StubEntitlementNeverGrants)
{
  ResetCapabilities();
  explorer_pro::SetEntitlementSource(nullptr);
  TEST(!explorer_pro::IsEntitled(), ());
  settings::Set("ExplorerPro.Entitled", true);
  TEST(!explorer_pro::IsEntitled(), ());
  settings::Delete("ExplorerPro.Entitled");
}

UNIT_TEST(ExplorerPro_GateUnavailableNotEntitled)
{
  ResetCapabilities();
  EntitlementSourceScope scope(nullptr);
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
}

UNIT_TEST(ExplorerPro_GateUnavailableEntitled)
{
  ResetCapabilities();
  FakeEntitlementSource entitled(true);
  EntitlementSourceScope scope(&entitled);
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
}

UNIT_TEST(ExplorerPro_GateAvailableNotEntitled)
{
  ResetCapabilities();
  CapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  EntitlementSourceScope scope(nullptr);
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
}

UNIT_TEST(ExplorerPro_GateAvailableEntitled)
{
  ResetCapabilities();
  CapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  FakeEntitlementSource entitled(true);
  EntitlementSourceScope scope(&entitled);
  TEST(explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
}

UNIT_TEST(ExplorerPro_GateMatrixAllCapabilities)
{
  ResetCapabilities();
  explorer_pro::Capability const capabilities[] = {
    explorer_pro::Capability::GpxImport,
    explorer_pro::Capability::GpxExport,
    explorer_pro::Capability::AdvancedTrackManagement,
  };

  for (auto const capability : capabilities)
  {
    {
      ResetCapabilities();
      EntitlementSourceScope scope(nullptr);
      TEST(!explorer_pro::IsCapabilityEnabled(capability), ());
    }
    {
      ResetCapabilities();
      FakeEntitlementSource entitled(true);
      EntitlementSourceScope scope(&entitled);
      TEST(!explorer_pro::IsCapabilityEnabled(capability), ());
    }
    {
      ResetCapabilities();
      CapabilityAvailabilityScope availability(capability, true);
      EntitlementSourceScope scope(nullptr);
      TEST(!explorer_pro::IsCapabilityEnabled(capability), ());
    }
    {
      ResetCapabilities();
      CapabilityAvailabilityScope availability(capability, true);
      FakeEntitlementSource entitled(true);
      EntitlementSourceScope scope(&entitled);
      TEST(explorer_pro::IsCapabilityEnabled(capability), ());
    }
  }
}

UNIT_TEST(ExplorerPro_DebugEntitlementSourceUsed)
{
  ResetCapabilities();
  CapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  RestoreStubOnExit restore;
  explorer_pro::InstallDebugEntitlementSource();
  TEST(explorer_pro::IsEntitled(), ());
  TEST(explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
}

UNIT_TEST(ExplorerPro_DebugEntitlementSourceNotUsed)
{
  ResetCapabilities();
  CapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  explorer_pro::SetEntitlementSource(nullptr);
  TEST(!explorer_pro::IsEntitled(), ());
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
}

UNIT_TEST(ExplorerPro_DebugEntitlementSourceStubRestored)
{
  ResetCapabilities();
  CapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  RestoreStubOnExit restore;
  explorer_pro::InstallDebugEntitlementSource();
  TEST(explorer_pro::IsEntitled(), ());
  explorer_pro::SetEntitlementSource(nullptr);
  TEST(!explorer_pro::IsEntitled(), ());
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
}

UNIT_TEST(ExplorerPro_FrozenKeepsEnabledState)
{
  ResetCapabilities();
  ExplorerProUnfreezeOnExit unfreeze;
  CapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  RestoreStubOnExit restore;
  explorer_pro::InstallDebugEntitlementSource();
  TEST(explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
  explorer_pro::FreezeConfiguration();
  explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxImport, false);
  explorer_pro::SetEntitlementSource(nullptr);
  TEST(explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
  explorer_pro::UnfreezeConfigurationForTesting();
}

UNIT_TEST(ExplorerPro_FrozenIgnoresDebugInstall)
{
  ResetCapabilities();
  ExplorerProUnfreezeOnExit unfreeze;
  CapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  explorer_pro::SetEntitlementSource(nullptr);
  TEST(!explorer_pro::IsEntitled(), ());
  explorer_pro::FreezeConfiguration();
  explorer_pro::InstallDebugEntitlementSource();
  explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxExport, true);
  TEST(!explorer_pro::IsEntitled(), ());
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
  TEST(!explorer_pro::IsCapabilityAvailable(explorer_pro::Capability::GpxExport), ());
  explorer_pro::UnfreezeConfigurationForTesting();
}
