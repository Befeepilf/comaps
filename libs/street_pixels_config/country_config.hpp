#pragma once

#include "base/exception.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace street_pixels
{
inline constexpr char kCountryPoliciesRelativePath[] = "street_pixels/country_policies.json";
inline constexpr uint32_t kSupportedSchemaVersion = 1;

struct PlaceBoundaryPolicy
{
  bool m_enabled = false;
  std::vector<std::string> m_placeTypes;
};

struct CountryPolicy
{
  bool m_configured = false;
  std::string m_isoCode;
  std::vector<std::string> m_mwmRootIds;
  // Priority order: index 0 is highest priority (SPD-023 / §8.8).
  std::vector<int> m_subdivisionAdminLevels;
  std::vector<int> m_settlementAdminLevels;
  PlaceBoundaryPolicy m_placeBoundaries;
};

// Versioned country exploration-area policy (SPD-023). Unknown ISO / MWM ids
// resolve to UnconfiguredPolicy (configured=false, empty levels) — never invent
// grids. No numeric floor fields exist or are applied (SPD-024).
class CountryConfig
{
public:
  DECLARE_EXCEPTION(Exception, RootException);

  static CountryConfig LoadFromString(std::string const & json);
  static CountryConfig LoadFromFile(std::string const & path);

  uint32_t GetPolicyVersion() const { return m_policyVersion; }
  uint32_t GetSchemaVersion() const { return m_schemaVersion; }

  CountryPolicy const & GetByIso(std::string const & isoCode) const;
  CountryPolicy const & GetByMwmId(std::string const & countryId) const;

  static CountryPolicy const & UnconfiguredPolicy();

private:
  uint32_t m_policyVersion = 0;
  uint32_t m_schemaVersion = 0;
  std::map<std::string, CountryPolicy> m_byIso;
  std::map<std::string, std::string> m_mwmRootToIso;
};
}  // namespace street_pixels
