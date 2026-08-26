#include "map/competition_upload_payload.hpp"

#include "coding/serdes_json.hpp"
#include "coding/writer.hpp"

#include <string>

bool CompetitionUploadPayloadIsEmpty(CompetitionUploadPayload const & payload)
{
  return payload.m_areas.empty() && payload.m_weeklyCities.empty();
}

std::string SerializeCompetitionUploadPayload(CompetitionUploadPayload const & payload)
{
  std::string json;
  {
    using Sink = MemWriter<std::string>;
    Sink sink(json);
    coding::SerializerJson<Sink> ser(sink);
    ser(payload);
  }
  return json;
}
