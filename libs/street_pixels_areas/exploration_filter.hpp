#pragma once

#include "street_pixels_areas/areas_types.hpp"

#include "street_pixels_config/country_config.hpp"

#include <optional>
#include <string>

namespace street_pixels
{
enum class RejectReason
{
  Accepted = 0,
  ThreeBox,
  PlaceNodeInvented,
  Unnamed,
  EmptyRings,
  InvalidRing,
  PolicyMismatch,
  UnconfiguredCountry,
};

struct FilterResult
{
  RejectReason m_reason = RejectReason::Accepted;
  std::optional<ExplorationArea> m_area;
};

// Admit only true closed named rings selected by country policy (SPD-024).
// Never invent place-node polygons or three-box geometry (SPD-020/025, §8.3).
FilterResult FilterExplorationCandidate(AreaCandidateInput const & input, CountryPolicy const & policy);

char const * DebugPrint(RejectReason reason);
}  // namespace street_pixels
