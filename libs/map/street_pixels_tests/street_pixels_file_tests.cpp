#include "testing/testing.hpp"

#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/mmap_reader.hpp"
#include "coding/writer.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace
{
std::string TestPixPath(std::string const & name)
{
  return base::JoinPath(GetPlatform().WritableDir(), name);
}

void RemoveIfExists(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
}

void WriteRawBytes(std::string const & path, std::vector<uint8_t> const & bytes)
{
  FileWriter writer(path, FileWriter::OP_WRITE_TRUNCATE);
  if (!bytes.empty())
    writer.Write(bytes.data(), bytes.size());
  writer.Flush();
}

std::vector<uint8_t> ReadAllBytes(std::string const & path)
{
  FileReader reader(path);
  std::vector<uint8_t> bytes(static_cast<size_t>(reader.Size()));
  if (!bytes.empty())
    reader.Read(0, bytes.data(), bytes.size());
  return bytes;
}

std::vector<uint8_t> EncodeHeaderBytes(uint16_t formatVersion, uint16_t flags, int64_t mapDataVersion)
{
  std::vector<uint8_t> bytes;
  MemWriter<std::vector<uint8_t>> writer(bytes);
  street_pixels_file::WriteHeader(writer, mapDataVersion, formatVersion, flags);
  TEST_EQUAL(bytes.size(), street_pixels_file::kHeaderSize, ());
  return bytes;
}

void WriteHeaderedRawWords(std::string const & path, int64_t mapDataVersion, std::vector<int64_t> const & words)
{
  auto bytes = EncodeHeaderBytes(street_pixels_file::kFormatVersionV1, street_pixels_file::kFlagsHasHeaderBit,
                                 mapDataVersion);
  for (int64_t word : words)
  {
    uint8_t raw[sizeof(int64_t)];
    std::memcpy(raw, &word, sizeof(word));
    bytes.insert(bytes.end(), raw, raw + sizeof(raw));
  }
  WriteRawBytes(path, bytes);
}

void WriteLegacyRawWords(std::string const & path, std::vector<int64_t> const & words)
{
  std::vector<uint8_t> bytes;
  bytes.reserve(words.size() * sizeof(int64_t));
  for (int64_t word : words)
  {
    uint8_t raw[sizeof(int64_t)];
    std::memcpy(raw, &word, sizeof(word));
    bytes.insert(bytes.end(), raw, raw + sizeof(raw));
  }
  WriteRawBytes(path, bytes);
}

int64_t RawExploredWord(int64_t pixelId)
{
  return (pixelId & 0x7FFFFFFFFFFFFFFFLL) | static_cast<int64_t>(0x8000000000000000ULL);
}

std::span<df::StreetPixel> MapPixBody(MmapReader & reader)
{
  TEST_GREATER_OR_EQUAL(reader.Size(), street_pixels_file::kHeaderSize, ());
  size_t const count =
      static_cast<size_t>((reader.Size() - street_pixels_file::kHeaderSize) / sizeof(df::StreetPixel));
  auto * body = reinterpret_cast<df::StreetPixel *>(reader.Data() + street_pixels_file::kHeaderSize);
  return {body, count};
}
}  // namespace

UNIT_TEST(StreetPixelsFile_ProbeHeaderedLegacyUnsupported)
{
  {
    auto bytes = EncodeHeaderBytes(1, street_pixels_file::kFlagsHasHeaderBit, 42);
    int64_t id = 7;
    uint8_t raw[sizeof(id)];
    std::memcpy(raw, &id, sizeof(id));
    bytes.insert(bytes.end(), raw, raw + sizeof(raw));

    auto const probe = street_pixels_file::Probe(bytes.data(), bytes.size());
    TEST_EQUAL(static_cast<int>(probe.kind), static_cast<int>(street_pixels_file::FileKind::HeaderedV1), ());
    TEST_EQUAL(probe.header.mapDataVersion, 42, ());
    TEST_EQUAL(probe.header.formatVersion, 1, ());
  }

  {
    std::vector<uint8_t> legacy(16, 0);
    auto const probe = street_pixels_file::Probe(legacy.data(), legacy.size());
    TEST_EQUAL(static_cast<int>(probe.kind), static_cast<int>(street_pixels_file::FileKind::Legacy), ());
  }

  {
    auto bytes = EncodeHeaderBytes(2, street_pixels_file::kFlagsHasHeaderBit, 99);
    auto const probe = street_pixels_file::Probe(bytes.data(), bytes.size());
    TEST_EQUAL(static_cast<int>(probe.kind), static_cast<int>(street_pixels_file::FileKind::UnsupportedFormat), ());
    TEST_EQUAL(probe.header.formatVersion, 2, ());
  }

  {
    std::vector<uint8_t> corrupt(3, 0xab);
    auto const probe = street_pixels_file::Probe(corrupt.data(), corrupt.size());
    TEST_EQUAL(static_cast<int>(probe.kind), static_cast<int>(street_pixels_file::FileKind::Corrupt), ());
  }
}

UNIT_TEST(StreetPixelsFile_FlagsBit0ClearedTreatedAsLegacy)
{
  auto bytes = EncodeHeaderBytes(1, 0, 12345);
  TEST_EQUAL(bytes.size(), street_pixels_file::kHeaderSize, ());
  TEST(!street_pixels_file::LooksLikeHeader(bytes.data(), bytes.size()), ());
  auto const probe = street_pixels_file::Probe(bytes.data(), bytes.size());
  TEST_EQUAL(static_cast<int>(probe.kind), static_cast<int>(street_pixels_file::FileKind::Legacy), ());
}

UNIT_TEST(StreetPixelsFile_RoundTripHeaderedPreservesExploredMsbs)
{
  std::string const path = TestPixPath("sp015_roundtrip.pix");
  RemoveIfExists(path);

  int64_t constexpr kVersion = 20260803;
  std::vector<int64_t> const words = {11, RawExploredWord(22), 33, RawExploredWord(44)};
  WriteHeaderedRawWords(path, kVersion, words);

  MmapReader reader(path, MmapReader::Advice::Normal, true);
  auto header = street_pixels_file::ReadHeader(reader.Data());
  TEST_EQUAL(header.mapDataVersion, kVersion, ());
  auto body = MapPixBody(reader);
  TEST_EQUAL(body.size(), words.size(), ());
  TEST_EQUAL(body[0].GetPixelId(), 11, ());
  TEST(!body[0].IsExplored(), ());
  TEST_EQUAL(body[1].GetPixelId(), 22, ());
  TEST(body[1].IsExplored(), ());
  TEST_EQUAL(body[2].GetPixelId(), 33, ());
  TEST(!body[2].IsExplored(), ());
  TEST_EQUAL(body[3].GetPixelId(), 44, ());
  TEST(body[3].IsExplored(), ());

  auto const onDisk = ReadAllBytes(path);
  TEST_EQUAL(onDisk.size(), street_pixels_file::kHeaderSize + words.size() * sizeof(int64_t), ());

  RemoveIfExists(path);
}

UNIT_TEST(StreetPixelsFile_SaveUnexploredIdsStampsVersion)
{
  std::string const path = TestPixPath("sp015_save.pix");
  RemoveIfExists(path);

  int64_t constexpr kVersion = 150309;
  std::set<int64_t> ids = {100, 200, 300};
  TEST(street_pixels_file::SaveUnexploredIds(path, ids, kVersion), ());

  auto const probe = street_pixels_file::ProbeFile(path);
  TEST_EQUAL(static_cast<int>(probe.kind), static_cast<int>(street_pixels_file::FileKind::HeaderedV1), ());
  TEST_EQUAL(probe.header.mapDataVersion, kVersion, ());

  MmapReader reader(path, MmapReader::Advice::Normal, false);
  auto body = MapPixBody(reader);
  TEST_EQUAL(body.size(), 3, ());
  TEST_EQUAL(body[0].GetPixelId(), 100, ());
  TEST(!body[0].IsExplored(), ());
  TEST_EQUAL(body[1].GetPixelId(), 200, ());
  TEST_EQUAL(body[2].GetPixelId(), 300, ());

  RemoveIfExists(path);
}

UNIT_TEST(StreetPixelsFile_LegacyMigratesPreservingExplored)
{
  std::string const path = TestPixPath("sp015_legacy.pix");
  RemoveIfExists(path);

  std::vector<int64_t> const words = {5, RawExploredWord(6), RawExploredWord(7), 8};
  WriteLegacyRawWords(path, words);
  auto const before = ReadAllBytes(path);
  TEST_EQUAL(before.size(), words.size() * sizeof(int64_t), ());

  int64_t constexpr kVersion = 4242;
  street_pixels_file::MigrateLegacyFile(path, kVersion);

  auto const probe = street_pixels_file::ProbeFile(path);
  TEST_EQUAL(static_cast<int>(probe.kind), static_cast<int>(street_pixels_file::FileKind::HeaderedV1), ());
  TEST_EQUAL(probe.header.mapDataVersion, kVersion, ());

  auto const after = ReadAllBytes(path);
  TEST_EQUAL(after.size(), street_pixels_file::kHeaderSize + before.size(), ());
  TEST(std::equal(before.begin(), before.end(), after.begin() + street_pixels_file::kHeaderSize), ());

  MmapReader reader(path, MmapReader::Advice::Normal, false);
  auto body = MapPixBody(reader);
  TEST_EQUAL(body.size(), 4, ());
  TEST_EQUAL(body[0].GetPixelId(), 5, ());
  TEST(!body[0].IsExplored(), ());
  TEST_EQUAL(body[1].GetPixelId(), 6, ());
  TEST(body[1].IsExplored(), ());
  TEST_EQUAL(body[2].GetPixelId(), 7, ());
  TEST(body[2].IsExplored(), ());
  TEST_EQUAL(body[3].GetPixelId(), 8, ());
  TEST(!body[3].IsExplored(), ());

  RemoveIfExists(path);
}

UNIT_TEST(StreetPixelsFile_UnsupportedFormatRejectedWithoutRewrite)
{
  std::string const path = TestPixPath("sp015_unsupported.pix");
  RemoveIfExists(path);

  auto bytes = EncodeHeaderBytes(9, street_pixels_file::kFlagsHasHeaderBit, 77);
  int64_t id = RawExploredWord(55);
  uint8_t raw[sizeof(id)];
  std::memcpy(raw, &id, sizeof(id));
  bytes.insert(bytes.end(), raw, raw + sizeof(raw));
  WriteRawBytes(path, bytes);
  auto const original = ReadAllBytes(path);

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  bool threw = false;
  try
  {
    manager.LoadStreetPixelsFromFile("sp015_unsupported", 77);
  }
  catch (street_pixels_file::UnsupportedStreetPixelsFormat const &)
  {
    threw = true;
  }
  TEST(threw, ());

  auto const after = ReadAllBytes(path);
  TEST_EQUAL(after, original, ());
  TEST(!street_pixels_file::MayRecoverByDerive(street_pixels_file::ProbeFile(path).kind), ());

  RemoveIfExists(path);
}

UNIT_TEST(StreetPixelsManager_LoadHeaderedSetsMapDataVersion)
{
  std::string const countryId = "sp015_mgr_version";
  std::string const path = GetPlatform().WritablePathForFile(countryId + ".pix");
  RemoveIfExists(path);

  int64_t constexpr kVersion = 990011;
  WriteHeaderedRawWords(path, kVersion, {RawExploredWord(1), 2});

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.LoadStreetPixelsFromFile(countryId, kVersion);
  TEST_EQUAL(manager.GetPixMapDataVersion(), kVersion, ());
  TEST(manager.IsPixelExploredForTesting(1), ());
  TEST(!manager.IsPixelExploredForTesting(2), ());

  manager.ClearPixels();
  TEST_EQUAL(manager.GetPixMapDataVersion(), 0, ());

  RemoveIfExists(path);
}

UNIT_TEST(StreetPixelsManager_LegacyLoadMigratesAndStampsVersion)
{
  std::string const countryId = "sp015_mgr_legacy";
  std::string const path = GetPlatform().WritablePathForFile(countryId + ".pix");
  RemoveIfExists(path);

  WriteLegacyRawWords(path, {RawExploredWord(9), 10});
  int64_t constexpr kVersion = 112233;

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.LoadStreetPixelsFromFile(countryId, kVersion);

  TEST_EQUAL(manager.GetPixMapDataVersion(), kVersion, ());
  TEST(manager.IsPixelExploredForTesting(9), ());
  TEST(!manager.IsPixelExploredForTesting(10), ());

  auto const probe = street_pixels_file::ProbeFile(path);
  TEST_EQUAL(static_cast<int>(probe.kind), static_cast<int>(street_pixels_file::FileKind::HeaderedV1), ());
  TEST_EQUAL(probe.header.mapDataVersion, kVersion, ());

  RemoveIfExists(path);
}

UNIT_TEST(StreetPixelsFile_MayRecoverByDeriveOnlyCorrupt)
{
  TEST(!street_pixels_file::MayRecoverByDerive(street_pixels_file::FileKind::HeaderedV1), ());
  TEST(!street_pixels_file::MayRecoverByDerive(street_pixels_file::FileKind::Legacy), ());
  TEST(!street_pixels_file::MayRecoverByDerive(street_pixels_file::FileKind::UnsupportedFormat), ());
  TEST(street_pixels_file::MayRecoverByDerive(street_pixels_file::FileKind::Corrupt), ());
}

UNIT_TEST(StreetPixelsFile_MigrateNonLegacyLeavesFileIntact)
{
  std::string const path = TestPixPath("sp015_migrate_nonlegacy.pix");
  RemoveIfExists(path);

  int64_t constexpr kVersion = 77;
  WriteHeaderedRawWords(path, kVersion, {RawExploredWord(3), 4});
  auto const original = ReadAllBytes(path);

  bool threw = false;
  try
  {
    street_pixels_file::MigrateLegacyFile(path, 55);
  }
  catch (street_pixels_file::StreetPixelsMigrationException const &)
  {
    threw = true;
  }
  TEST(threw, ());

  auto const after = ReadAllBytes(path);
  TEST_EQUAL(after, original, ());
  auto const probe = street_pixels_file::ProbeFile(path);
  TEST_EQUAL(static_cast<int>(probe.kind), static_cast<int>(street_pixels_file::FileKind::HeaderedV1), ());
  TEST_EQUAL(probe.header.mapDataVersion, kVersion, ());

  RemoveIfExists(path);
}
