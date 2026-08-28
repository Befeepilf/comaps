#pragma once

namespace explorer_pro
{
enum class Capability
{
  GpxImport,
  GpxExport,
  AdvancedTrackManagement,
};

class EntitlementSource
{
public:
  virtual ~EntitlementSource() = default;
  virtual bool IsEntitled() const = 0;
};

class StubEntitlementSource : public EntitlementSource
{
public:
  bool IsEntitled() const override;
};

class DebugEntitlementSource : public EntitlementSource
{
public:
  bool IsEntitled() const override;
};

void SetCapabilityAvailable(Capability capability, bool available);
bool IsCapabilityAvailable(Capability capability);

bool IsEntitled();

void SetEntitlementSource(EntitlementSource * source);
void InstallDebugEntitlementSource();

bool IsCapabilityEnabled(Capability capability);
}
