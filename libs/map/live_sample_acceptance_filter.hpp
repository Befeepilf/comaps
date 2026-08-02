#pragma once

#include "platform/location.hpp"

#include <string>

inline constexpr double kMaxHorizontalAccuracyMeters = 25.0;
inline constexpr double kMaxSampleAgeSeconds = 120.0;
inline constexpr double kMaxImpliedSpeedMps = 50.0 / 3.6;
inline constexpr double kMaxJumpMeters = 200.0;

enum class SampleRejectReason
{
  None,
  Invalid,
  MissingAccuracy,
  Accuracy,
  Stale,
  ImpliedSpeed,
  Teleport,
};

struct SampleAcceptanceResult
{
  bool accepted = false;
  SampleRejectReason reason = SampleRejectReason::None;
};

class LiveSampleAcceptanceFilter
{
public:
  SampleAcceptanceResult Evaluate(location::GpsInfo const & info, double nowSec);
  void ResetAcceptedReference();
  bool HasAcceptedReference() const;
  SampleRejectReason GetLastRejectReason() const;

private:
  bool m_hasReference = false;
  location::GpsInfo m_lastAccepted;
  SampleRejectReason m_lastRejectReason = SampleRejectReason::None;
};

std::string DebugPrint(SampleRejectReason reason);
