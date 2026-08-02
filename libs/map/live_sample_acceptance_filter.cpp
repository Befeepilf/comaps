#include "map/live_sample_acceptance_filter.hpp"

#include "geometry/distance_on_sphere.hpp"

namespace
{
SampleAcceptanceResult Reject(SampleRejectReason reason)
{
  SampleAcceptanceResult result;
  result.accepted = false;
  result.reason = reason;
  return result;
}
}  // namespace

SampleAcceptanceResult LiveSampleAcceptanceFilter::Evaluate(location::GpsInfo const & info, double nowSec)
{
  if (!info.IsValid())
  {
    m_lastRejectReason = SampleRejectReason::Invalid;
    return Reject(SampleRejectReason::Invalid);
  }

  if (info.m_timestamp <= 0.0)
  {
    m_lastRejectReason = SampleRejectReason::Invalid;
    return Reject(SampleRejectReason::Invalid);
  }

  if (info.m_horizontalAccuracy <= 0.0)
  {
    m_lastRejectReason = SampleRejectReason::MissingAccuracy;
    return Reject(SampleRejectReason::MissingAccuracy);
  }

  if (info.m_horizontalAccuracy > kMaxHorizontalAccuracyMeters)
  {
    m_lastRejectReason = SampleRejectReason::Accuracy;
    return Reject(SampleRejectReason::Accuracy);
  }

  if (nowSec - info.m_timestamp > kMaxSampleAgeSeconds)
  {
    m_lastRejectReason = SampleRejectReason::Stale;
    return Reject(SampleRejectReason::Stale);
  }

  if (m_hasReference)
  {
    if (info.m_timestamp <= m_lastAccepted.m_timestamp)
    {
      m_lastRejectReason = SampleRejectReason::Stale;
      return Reject(SampleRejectReason::Stale);
    }

    double const distanceMeters =
        ms::DistanceOnEarth(m_lastAccepted.m_latitude, m_lastAccepted.m_longitude, info.m_latitude, info.m_longitude);

    if (distanceMeters > kMaxJumpMeters)
    {
      m_lastRejectReason = SampleRejectReason::Teleport;
      return Reject(SampleRejectReason::Teleport);
    }

    double const dtSec = info.m_timestamp - m_lastAccepted.m_timestamp;
    if (dtSec > 0.0)
    {
      double const impliedSpeedMps = distanceMeters / dtSec;
      if (impliedSpeedMps > kMaxImpliedSpeedMps)
      {
        m_lastRejectReason = SampleRejectReason::ImpliedSpeed;
        return Reject(SampleRejectReason::ImpliedSpeed);
      }
    }
  }

  m_lastAccepted = info;
  m_hasReference = true;
  m_lastRejectReason = SampleRejectReason::None;

  SampleAcceptanceResult result;
  result.accepted = true;
  result.reason = SampleRejectReason::None;
  return result;
}

void LiveSampleAcceptanceFilter::ResetAcceptedReference()
{
  m_hasReference = false;
  m_lastAccepted = {};
  m_lastRejectReason = SampleRejectReason::None;
}

bool LiveSampleAcceptanceFilter::HasAcceptedReference() const { return m_hasReference; }

SampleRejectReason LiveSampleAcceptanceFilter::GetLastRejectReason() const { return m_lastRejectReason; }

std::string DebugPrint(SampleRejectReason reason)
{
  switch (reason)
  {
  case SampleRejectReason::None: return "None";
  case SampleRejectReason::Invalid: return "Invalid";
  case SampleRejectReason::MissingAccuracy: return "MissingAccuracy";
  case SampleRejectReason::Accuracy: return "Accuracy";
  case SampleRejectReason::Stale: return "Stale";
  case SampleRejectReason::ImpliedSpeed: return "ImpliedSpeed";
  case SampleRejectReason::Teleport: return "Teleport";
  }
  return "Unknown";
}
