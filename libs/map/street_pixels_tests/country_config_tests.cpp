#include "testing/testing.hpp"

#include "street_pixels_config/country_config.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <string>
#include <vector>

namespace
{
std::string const kFinlandFixture = R"({
  "policy_version": 1,
  "schema_version": 1,
  "countries": {
    "FI": {
      "mwm_root_ids": ["Finland"],
      "subdivision_admin_levels": [10, 9, 11],
      "settlement_admin_levels": [8],
      "place_boundaries": {
        "enabled": true,
        "place_types": ["neighbourhood", "quarter", "suburb"]
      }
    }
  }
})";

void ExpectFinlandPriority(street_pixels::CountryPolicy const & policy)
{
  TEST(policy.m_configured, ());
  TEST_EQUAL(policy.m_isoCode, "FI", ());
  TEST_EQUAL(policy.m_mwmRootIds, (std::vector<std::string>{"Finland"}), ());
  TEST_EQUAL(policy.m_subdivisionAdminLevels, (std::vector<int>{10, 9, 11}), ());
  TEST_EQUAL(policy.m_settlementAdminLevels, (std::vector<int>{8}), ());
  TEST(policy.m_placeBoundaries.m_enabled, ());
  TEST_EQUAL(policy.m_placeBoundaries.m_placeTypes,
             (std::vector<std::string>{"neighbourhood", "quarter", "suburb"}), ());
}
}  // namespace

UNIT_TEST(CountryConfig_FinlandFixturePriority)
{
  auto const config = street_pixels::CountryConfig::LoadFromString(kFinlandFixture);
  ExpectFinlandPriority(config.GetByIso("FI"));
}

UNIT_TEST(CountryConfig_MwmLeafAndRootLookup)
{
  auto const config = street_pixels::CountryConfig::LoadFromString(kFinlandFixture);

  ExpectFinlandPriority(config.GetByMwmId("Finland"));
  ExpectFinlandPriority(config.GetByMwmId("Finland_Southern Finland_Helsinki"));
  ExpectFinlandPriority(config.GetByMwmId("Finland_Northern Finland"));

  TEST(!config.GetByMwmId("FinlandX").m_configured, ());
  TEST(!config.GetByMwmId("Sweden").m_configured, ());
}

UNIT_TEST(CountryConfig_UnknownIsoAndMwmUnconfigured)
{
  auto const config = street_pixels::CountryConfig::LoadFromString(kFinlandFixture);

  auto const & byIso = config.GetByIso("SE");
  TEST(!byIso.m_configured, ());
  TEST(byIso.m_subdivisionAdminLevels.empty(), ());
  TEST(byIso.m_settlementAdminLevels.empty(), ());
  TEST(byIso.m_mwmRootIds.empty(), ());
  TEST(!byIso.m_placeBoundaries.m_enabled, ());
  TEST(byIso.m_placeBoundaries.m_placeTypes.empty(), ());

  auto const & byMwm = config.GetByMwmId("Germany_Berlin");
  TEST(!byMwm.m_configured, ());
  TEST(byMwm.m_subdivisionAdminLevels.empty(), ());

  auto const & unconfigured = street_pixels::CountryConfig::UnconfiguredPolicy();
  TEST(!unconfigured.m_configured, ());
  TEST(unconfigured.m_subdivisionAdminLevels.empty(), ());
}

UNIT_TEST(CountryConfig_InvalidJsonFails)
{
  bool threw = false;
  try
  {
    street_pixels::CountryConfig::LoadFromString("{ not json");
  }
  catch (street_pixels::CountryConfig::Exception const &)
  {
    threw = true;
  }
  TEST(threw, ());

  threw = false;
  try
  {
    street_pixels::CountryConfig::LoadFromString(R"({"policy_version":1})");
  }
  catch (street_pixels::CountryConfig::Exception const &)
  {
    threw = true;
  }
  TEST(threw, ("Missing schema_version and countries must fail."));
}

UNIT_TEST(CountryConfig_DuplicateMwmRootFails)
{
  std::string const json = R"({
    "policy_version": 1,
    "schema_version": 1,
    "countries": {
      "FI": {
        "mwm_root_ids": ["Finland"],
        "subdivision_admin_levels": [10],
        "settlement_admin_levels": [8]
      },
      "AX": {
        "mwm_root_ids": ["Finland"],
        "subdivision_admin_levels": [10],
        "settlement_admin_levels": [8]
      }
    }
  })";

  bool threw = false;
  try
  {
    street_pixels::CountryConfig::LoadFromString(json);
  }
  catch (street_pixels::CountryConfig::Exception const &)
  {
    threw = true;
  }
  TEST(threw, ());
}

UNIT_TEST(CountryConfig_PolicyVersionReadable)
{
  auto const config = street_pixels::CountryConfig::LoadFromString(kFinlandFixture);
  TEST_EQUAL(config.GetPolicyVersion(), 1u, ());
  TEST_EQUAL(config.GetSchemaVersion(), 1u, ());
}

UNIT_TEST(CountryConfig_IgnoreFloorKeysNeverApply)
{
  std::string const json = R"({
    "policy_version": 1,
    "schema_version": 1,
    "countries": {
      "FI": {
        "mwm_root_ids": ["Finland"],
        "subdivision_admin_levels": [10, 9, 11],
        "settlement_admin_levels": [8],
        "min_pixel_count": 500,
        "min_area_m2": 10000,
        "privacy_floor_pixels": 3,
        "place_boundaries": {
          "enabled": true,
          "place_types": ["neighbourhood", "quarter", "suburb"],
          "min_pixel_count": 100
        }
      }
    }
  })";

  auto const config = street_pixels::CountryConfig::LoadFromString(json);
  ExpectFinlandPriority(config.GetByIso("FI"));
  // Floor keys are ignored; CountryPolicy has no floor fields to apply (SPD-024).
  TEST_EQUAL(config.GetPolicyVersion(), 1u, ());
}

UNIT_TEST(CountryConfig_UnsupportedSchemaVersionFails)
{
  std::string const json = R"({
    "policy_version": 1,
    "schema_version": 99,
    "countries": {}
  })";

  bool threw = false;
  try
  {
    street_pixels::CountryConfig::LoadFromString(json);
  }
  catch (street_pixels::CountryConfig::Exception const &)
  {
    threw = true;
  }
  TEST(threw, ("Unsupported schema_version must fail."));
}

UNIT_TEST(CountryConfig_InvalidIsoKeyFails)
{
  std::string const json = R"({
    "policy_version": 1,
    "schema_version": 1,
    "countries": {
      "fi": {
        "mwm_root_ids": ["Finland"],
        "subdivision_admin_levels": [10],
        "settlement_admin_levels": [8]
      }
    }
  })";

  bool threw = false;
  try
  {
    street_pixels::CountryConfig::LoadFromString(json);
  }
  catch (street_pixels::CountryConfig::Exception const &)
  {
    threw = true;
  }
  TEST(threw, ("Lowercase ISO key must fail."));
}

UNIT_TEST(CountryConfig_LongestMwmRootWins)
{
  std::string const json = R"({
    "policy_version": 1,
    "schema_version": 1,
    "countries": {
      "FI": {
        "mwm_root_ids": ["Finland"],
        "subdivision_admin_levels": [10, 9, 11],
        "settlement_admin_levels": [8]
      },
      "AX": {
        "mwm_root_ids": ["Finland_Aland"],
        "subdivision_admin_levels": [9],
        "settlement_admin_levels": [8]
      }
    }
  })";

  auto const config = street_pixels::CountryConfig::LoadFromString(json);
  TEST_EQUAL(config.GetByMwmId("Finland_Southern Finland_Helsinki").m_isoCode, "FI", ());
  TEST_EQUAL(config.GetByMwmId("Finland_Aland").m_isoCode, "AX", ());
  TEST_EQUAL(config.GetByMwmId("Finland_Aland_Mariehamn").m_isoCode, "AX", ());
  TEST_EQUAL(config.GetByMwmId("Finland_Aland_Mariehamn").m_subdivisionAdminLevels,
             (std::vector<int>{9}), ());
}

UNIT_TEST(CountryConfig_LoadShippedFinlandFixture)
{
  std::string const path =
      base::JoinPath(GetPlatform().ResourcesDir(), street_pixels::kCountryPoliciesRelativePath);
  auto const config = street_pixels::CountryConfig::LoadFromFile(path);
  TEST_EQUAL(config.GetPolicyVersion(), 1u, ());
  ExpectFinlandPriority(config.GetByIso("FI"));
  ExpectFinlandPriority(config.GetByMwmId("Finland_Southern Finland_Helsinki"));
}
