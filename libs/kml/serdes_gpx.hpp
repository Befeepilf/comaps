#pragma once

#include "kml/types.hpp"

#include "coding/parse_xml.hpp"
#include "coding/reader.hpp"

#include "geometry/point_with_altitude.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"

#include <cstddef>
#include <string>
#include <string_view>

class Writer;

namespace kml
{
void LogXmlParseFailurePrefix(Reader const & reader, std::string_view kind, size_t prefixBytes = 256);

namespace gpx
{

class GpxWriter
{
public:
  DECLARE_EXCEPTION(WriteGpxException, RootException);

  explicit GpxWriter(Writer & writer) : m_writer(writer) {}

  void Write(FileData const & fileData);

private:
  Writer & m_writer;
};

class SerializerGpx
{
public:
  DECLARE_EXCEPTION(SerializeException, RootException);

  explicit SerializerGpx(FileData const & fileData) : m_fileData(fileData) {}

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    GpxWriter gpxWriter(sink);
    gpxWriter.Write(m_fileData);
  }

private:
  FileData const & m_fileData;
};

class GpxParser
{
public:
  explicit GpxParser(FileData & data);
  bool Push(std::string name);
  void AddAttr(std::string_view attr, char const * value);
  std::string const & GetTagFromEnd(size_t n) const;
  void Pop(std::string_view tag);
  void CharData(std::string & value);
  static std::optional<uint32_t> ParseColorFromHexString(std::string_view colorStr);
  bool SawGpxRoot() const { return m_sawGpx; }

private:
  enum GeometryType
  {
    GEOMETRY_TYPE_UNKNOWN,
    GEOMETRY_TYPE_POINT,
    GEOMETRY_TYPE_LINE
  };

  void ResetPoint();
  bool MakeValid();
  void ParseColor(std::string_view colorStr);
  void ParseGarminColor(std::string const & value);
  void ParseOsmandColor(std::string const & value);
  bool IsValidCoordinatesPosition() const;
  void CheckAndCorrectTimestamps();

  FileData & m_data;
  CategoryData m_compilationData;
  CategoryData * m_categoryData;  // never null

  std::vector<std::string> m_tags;
  GeometryType m_geometryType;
  MultiGeometry m_geometry;
  uint32_t m_color;
  uint32_t m_globalColor;  // To support OSMAnd extensions with single color per GPX file

  std::string m_name;
  std::string m_description;
  std::string m_comment;
  PredefinedColor m_predefinedColor;
  geometry::PointWithAltitude m_org;

  double m_lat;
  double m_lon;
  bool m_hasLat = false;
  bool m_hasLon = false;
  bool m_sawGpx = false;
  geometry::Altitude m_altitude;
  time_t m_timestamp;

  MultiGeometry::LineT m_line;
  MultiGeometry::TimeT m_timestamps;
  std::string m_customName;
  void ParseName(std::string const & value, std::string const & prevTag);
  void ParseDescription(std::string const & value, std::string const & prevTag);
  void ParseAltitude(std::string const & value);
  void ParseTimestamp(std::string const & value);
  std::string BuildDescription() const;
};

}  // namespace gpx

class DeserializerGpx
{
public:
  DECLARE_EXCEPTION(DeserializeException, RootException);

  explicit DeserializerGpx(FileData & fileData);

  template <typename ReaderType>
  void Deserialize(ReaderType const & reader)
  {
    NonOwningReaderSource src(reader);

    gpx::GpxParser parser(m_fileData);
    if (!ParseXML(src, parser, true) || !parser.SawGpxRoot())
    {
      LogXmlParseFailurePrefix(reader, "GPX", 256);
      MYTHROW(DeserializeException, ("Could not parse GPX."));
    }
  }

private:
  FileData & m_fileData;
};
}  // namespace kml
