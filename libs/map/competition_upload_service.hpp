#pragma once

#include "map/competition_upload_payload.hpp"

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

int64_t constexpr kCompetitionMinUploadIntervalSeconds = 900;
int64_t constexpr kCompetitionMaxJitterSeconds = 900;

class CompetitionUploadService
{
public:
  using NowFn = std::function<int64_t()>;
  using JitterFn = std::function<int64_t()>;
  using ConnectedFn = std::function<bool()>;
  using Headers = std::vector<std::pair<std::string, std::string>>;
  using PostFn = std::function<int(std::string const & url, std::string const & body, Headers const & headers)>;
  using SnapshotFn = std::function<CompetitionUploadPayload(int64_t nowUnix)>;

  CompetitionUploadService();
  CompetitionUploadService(NowFn nowFn, JitterFn jitterFn, ConnectedFn connectedFn, PostFn postFn,
                           SnapshotFn snapshotFn);

  void SetSnapshotFn(SnapshotFn snapshotFn);

  void MarkPending();
  void MaybeUpload();

private:
  int64_t ClampedJitter() const;
  uint64_t LoadNextAllowedUnlocked() const;
  bool LoadPendingUnlocked() const;

  NowFn m_nowFn;
  JitterFn m_jitterFn;
  ConnectedFn m_connectedFn;
  PostFn m_postFn;
  SnapshotFn m_snapshotFn;
  mutable std::mutex m_mutex;
  bool m_uploadInFlight = false;
  bool m_markedWhileInFlight = false;
};
