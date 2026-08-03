#include "street_pixels_config/country_config.hpp"

#include "coding/file_reader.hpp"

#include "cppjansson/cppjansson.hpp"

#include <utility>

namespace street_pixels
{
namespace
{
PlaceBoundaryPolicy ParsePlaceBoundaries(json_t * countryObj)
{
  PlaceBoundaryPolicy policy;
  json_t * placeObj = base::GetJSONOptionalField(countryObj, "place_boundaries");
  if (!placeObj)
    return policy;
  if (!json_is_object(placeObj))
    MYTHROW(CountryConfig::Exception, ("place_boundaries must be a JSON object."));

  FromJSONObjectOptionalField(placeObj, "enabled", policy.m_enabled);
  FromJSONObjectOptionalField(placeObj, "place_types", policy.m_placeTypes);
  return policy;
}

CountryPolicy ParseCountryPolicy(std::string const & isoCode, json_t * countryObj)
{
  if (!json_is_object(countryObj))
    MYTHROW(CountryConfig::Exception, ("Country entry for", isoCode, "must be a JSON object."));

  CountryPolicy policy;
  policy.m_configured = true;
  policy.m_isoCode = isoCode;
  FromJSONObject(countryObj, "mwm_root_ids", policy.m_mwmRootIds);
  FromJSONObject(countryObj, "subdivision_admin_levels", policy.m_subdivisionAdminLevels);
  FromJSONObject(countryObj, "settlement_admin_levels", policy.m_settlementAdminLevels);
  policy.m_placeBoundaries = ParsePlaceBoundaries(countryObj);
  return policy;
}

bool MwmIdMatchesRoot(std::string const & countryId, std::string const & root)
{
  return countryId == root || countryId.starts_with(root + "_");
}
}  // namespace

CountryPolicy const & CountryConfig::UnconfiguredPolicy()
{
  static CountryPolicy const kUnconfigured;
  return kUnconfigured;
}

CountryConfig CountryConfig::LoadFromString(std::string const & json)
{
  CountryConfig config;
  try
  {
    base::Json root(json);
    if (!json_is_object(root.get()))
      MYTHROW(Exception, ("Country policy root must be a JSON object."));

    FromJSONObject(root.get(), "policy_version", config.m_policyVersion);
    FromJSONObject(root.get(), "schema_version", config.m_schemaVersion);
    if (config.m_schemaVersion != kSupportedSchemaVersion)
    {
      MYTHROW(Exception, ("Unsupported schema_version", config.m_schemaVersion, "; supported is",
                          kSupportedSchemaVersion));
    }

    json_t * countries = base::GetJSONObligatoryField(root.get(), "countries");
    if (!json_is_object(countries))
      MYTHROW(Exception, ("countries must be a JSON object keyed by ISO 3166-1 alpha-2."));

    char const * isoKey = nullptr;
    json_t * countryObj = nullptr;
    json_object_foreach(countries, isoKey, countryObj)
    {
      std::string const isoCode(isoKey);
      if (isoCode.empty())
        MYTHROW(Exception, ("Empty ISO country key is not allowed."));
      if (config.m_byIso.contains(isoCode))
        MYTHROW(Exception, ("Duplicate ISO country key", isoCode));

      CountryPolicy policy = ParseCountryPolicy(isoCode, countryObj);
      for (std::string const & rootId : policy.m_mwmRootIds)
      {
        if (rootId.empty())
          MYTHROW(Exception, ("Empty mwm_root_ids entry for", isoCode));
        auto const [it, inserted] = config.m_mwmRootToIso.emplace(rootId, isoCode);
        if (!inserted)
        {
          MYTHROW(Exception, ("Duplicate mwm_root_ids entry", rootId, "for", isoCode, "and", it->second));
        }
      }
      config.m_byIso.emplace(isoCode, std::move(policy));
    }
  }
  catch (base::Json::Exception const & e)
  {
    MYTHROW(Exception, ("Failed to parse country policy JSON:", e.Msg()));
  }

  return config;
}

CountryConfig CountryConfig::LoadFromFile(std::string const & path)
{
  std::string json;
  try
  {
    FileReader(path).ReadAsString(json);
  }
  catch (RootException const & e)
  {
    MYTHROW(Exception, ("Failed to read country policy file", path, ":", e.Msg()));
  }
  return LoadFromString(json);
}

CountryPolicy const & CountryConfig::GetByIso(std::string const & isoCode) const
{
  auto const it = m_byIso.find(isoCode);
  if (it == m_byIso.end())
    return UnconfiguredPolicy();
  return it->second;
}

CountryPolicy const & CountryConfig::GetByMwmId(std::string const & countryId) const
{
  auto const exact = m_mwmRootToIso.find(countryId);
  if (exact != m_mwmRootToIso.end())
    return GetByIso(exact->second);

  for (auto const & [root, iso] : m_mwmRootToIso)
  {
    if (MwmIdMatchesRoot(countryId, root))
      return GetByIso(iso);
  }
  return UnconfiguredPolicy();
}
}  // namespace street_pixels
