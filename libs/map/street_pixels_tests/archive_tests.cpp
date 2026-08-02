#include "testing/testing.hpp"

#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/mmap_reader.hpp"
#include "coding/writer.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <cstdint>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace
{
std::string ArchiveTestPath(std::string const & name)
{
  return base::JoinPath(GetPlatform().WritableDir(), name);
}

void ArchiveRemoveIfExists(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
}

void ArchiveWriteRawBytes(std::string const & path, std::vector<uint8_t> const & bytes)
{
  FileWriter writer(path, FileWriter::OP_WRITE_TRUNCATE);
  if (!bytes.empty())
    writer.Write(bytes.data(), bytes.size());
  writer.Flush();
}

std::vector<uint8_t> ArchiveReadAllBytes(std::string const & path)
{
  FileReader reader(path);
  std::vector<uint8_t> bytes(static_cast<size_t>(reader.Size()));
  if (!bytes.empty())
    reader.Read(0, bytes.data(), bytes.size());
  return bytes;
}

std::vector<uint8_t> ArchiveEncodePixHeaderBytes(uint16_t formatVersion, uint16_t flags, int64_t mapDataVersion)
{
  std::vector<uint8_t> bytes;
  MemWriter<std::vector<uint8_t>> writer(bytes);
  street_pixels_file::WriteHeader(writer, mapDataVersion, formatVersion, flags);
  return bytes;
}

void ArchiveWriteHeaderedPixWords(std::string const & path, int64_t mapDataVersion, std::vector<int64_t> const & words,
                                  uint16_t formatVersion = street_pixels_file::kFormatVersionV2)
{
  auto bytes = ArchiveEncodePixHeaderBytes(formatVersion, street_pixels_file::kFlagsHasHeaderBit, mapDataVersion);
  for (int64_t word : words)
  {
    uint8_t raw[sizeof(int64_t)];
    std::memcpy(raw, &word, sizeof(word));
    bytes.insert(bytes.end(), raw, raw + sizeof(raw));
  }
  ArchiveWriteRawBytes(path, bytes);
}

std::vector<int64_t> ArchiveReadPixBodyWords(std::string const & path)
{
  MmapReader reader(path);
  TEST_GREATER_OR_EQUAL(reader.Size(), street_pixels_file::kHeaderSize, ());
  size_t const count =
      static_cast<size_t>((reader.Size() - street_pixels_file::kHeaderSize) / sizeof(int64_t));
  std::vector<int64_t> words(count);
  if (count > 0)
    std::memcpy(words.data(), reader.Data() + street_pixels_file::kHeaderSize, count * sizeof(int64_t));
  return words;
}

size_t ArchiveCountExploredWords(std::vector<int64_t> const & words)
{
  size_t explored = 0;
  for (int64_t word : words)
  {
    if ((word & street_pixels_file::kExploredBit) != 0)
      ++explored;
  }
  return explored;
}
}  // namespace

UNIT_TEST(Archive_Roundtrip)
{
  std::string const path = ArchiveTestPath("sp018_roundtrip.pixr");
  ArchiveRemoveIfExists(path);

  street_pixels_file::ExploredEverLiveMap original;
  original[101] = true;
  original[202] = false;
  original[303] = true;

  TEST(street_pixels_file::SaveExploredArchive(path, original, 17), ());
  TEST(Platform::IsFileExistsByFullPath(path), ());

  auto const loaded = street_pixels_file::LoadExploredArchive(path);
  TEST(loaded.has_value(), ());
  TEST_EQUAL(loaded->size(), 3, ());
  TEST(loaded->at(101), ());
  TEST(!loaded->at(202), ());
  TEST(loaded->at(303), ());

  FileReader reader(path);
  TEST_GREATER_OR_EQUAL(reader.Size(), street_pixels_file::kHeaderSize, ());
  uint8_t headerBytes[street_pixels_file::kHeaderSize] = {};
  reader.Read(0, headerBytes, street_pixels_file::kHeaderSize);
  auto const header = street_pixels_file::ReadHeader(headerBytes);
  TEST_EQUAL(header.magic, street_pixels_file::kArchiveMagic, ());
  TEST_EQUAL(header.formatVersion, street_pixels_file::kFormatVersionV1, ());
  TEST_EQUAL(header.flags, street_pixels_file::kFlagsHasHeaderBit, ());
  TEST_EQUAL(header.mapDataVersion, 17, ());
  TEST_EQUAL(header.reserved, 0, ());

  ArchiveRemoveIfExists(path);
}

UNIT_TEST(Archive_SizeMuchSmallerThanFull)
{
  std::string const pixPath = ArchiveTestPath("sp018_size.pix");
  std::string const archivePath = ArchiveTestPath("sp018_size.pixr");
  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);

  size_t constexpr kUniverse = 5000;
  std::vector<int64_t> words;
  words.reserve(kUniverse);
  street_pixels_file::ExploredEverLiveMap explored;
  for (size_t i = 1; i <= kUniverse; ++i)
  {
    bool const isExplored = (i % 200) == 0;
    bool const everLive = isExplored && ((i % 400) == 0);
    words.push_back(street_pixels_file::PackPixelEntry(static_cast<int64_t>(i), isExplored, everLive));
    if (isExplored)
      explored[static_cast<int64_t>(i)] = everLive;
  }
  TEST_EQUAL(explored.size(), 25, ());

  ArchiveWriteHeaderedPixWords(pixPath, 3, words);
  TEST(street_pixels_file::SaveExploredArchive(archivePath, explored, 3), ());

  uint64_t pixSize = 0;
  uint64_t archiveSize = 0;
  TEST(Platform::GetFileSizeByFullPath(pixPath, pixSize), ());
  TEST(Platform::GetFileSizeByFullPath(archivePath, archiveSize), ());
  TEST_GREATER(pixSize, archiveSize * 10, (pixSize, archiveSize));
  TEST_EQUAL(archiveSize, street_pixels_file::kHeaderSize + explored.size() * sizeof(int64_t), ());

  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);
}

UNIT_TEST(Archive_CleanupArchivesExplored)
{
  std::string const countryId = "sp018_cleanup";
  std::string const pixPath = ArchiveTestPath(countryId + ".pix");
  std::string const archivePath = ArchiveTestPath(countryId + ".pixr");
  std::string const accountedPath = ArchiveTestPath(countryId + ".pixa");
  std::string const pixfPath = ArchiveTestPath(countryId + ".pixf");
  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);
  ArchiveRemoveIfExists(accountedPath);
  ArchiveRemoveIfExists(pixfPath);

  ArchiveWriteHeaderedPixWords(pixPath, 9,
                               {street_pixels_file::PackPixelEntry(11, true, true), 22,
                                street_pixels_file::PackPixelEntry(33, true, false), 44});
  ArchiveWriteRawBytes(accountedPath, {0x01});
  ArchiveWriteRawBytes(pixfPath, {0x02});

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.CleanupStreetPixelsForTesting(countryId);

  TEST(!Platform::IsFileExistsByFullPath(pixPath), ());
  TEST(!Platform::IsFileExistsByFullPath(accountedPath), ());
  TEST(!Platform::IsFileExistsByFullPath(pixfPath), ());
  TEST(Platform::IsFileExistsByFullPath(archivePath), ());

  auto const loaded = street_pixels_file::LoadExploredArchive(archivePath);
  TEST(loaded.has_value(), ());
  TEST_EQUAL(loaded->size(), 2, ());
  TEST(loaded->at(11), ());
  TEST(!loaded->at(33), ());

  ArchiveRemoveIfExists(archivePath);
}

UNIT_TEST(Archive_CleanupIdempotent)
{
  std::string const countryId = "sp018_idempotent";
  std::string const pixPath = ArchiveTestPath(countryId + ".pix");
  std::string const archivePath = ArchiveTestPath(countryId + ".pixr");
  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);

  ArchiveWriteHeaderedPixWords(pixPath, 4, {street_pixels_file::PackPixelEntry(7, true, true)});

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.CleanupStreetPixelsForTesting(countryId);
  TEST(Platform::IsFileExistsByFullPath(archivePath), ());
  auto const afterFirst = ArchiveReadAllBytes(archivePath);

  manager.CleanupStreetPixelsForTesting(countryId);
  TEST(Platform::IsFileExistsByFullPath(archivePath), ());
  TEST_EQUAL(ArchiveReadAllBytes(archivePath), afterFirst, ());
  TEST(!Platform::IsFileExistsByFullPath(pixPath), ());

  ArchiveRemoveIfExists(archivePath);
}

UNIT_TEST(Archive_ScanFailKeepsPix)
{
  std::string const countryId = "sp018_scan_fail";
  std::string const pixPath = ArchiveTestPath(countryId + ".pix");
  std::string const archivePath = ArchiveTestPath(countryId + ".pixr");
  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);

  street_pixels_file::ExploredEverLiveMap preexisting;
  preexisting[99] = true;
  TEST(street_pixels_file::SaveExploredArchive(archivePath, preexisting, 1), ());
  auto const archiveBefore = ArchiveReadAllBytes(archivePath);

  auto bytes = ArchiveEncodePixHeaderBytes(99, street_pixels_file::kFlagsHasHeaderBit, 7);
  int64_t const word = street_pixels_file::PackPixelEntry(55, true, true);
  uint8_t raw[sizeof(int64_t)];
  std::memcpy(raw, &word, sizeof(word));
  bytes.insert(bytes.end(), raw, raw + sizeof(raw));
  ArchiveWriteRawBytes(pixPath, bytes);
  auto const pixBefore = ArchiveReadAllBytes(pixPath);
  TEST(!street_pixels_file::ScanExploredEverLive(pixPath).has_value(), ());

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.CleanupStreetPixelsForTesting(countryId);

  TEST(Platform::IsFileExistsByFullPath(pixPath), ());
  TEST_EQUAL(ArchiveReadAllBytes(pixPath), pixBefore, ());
  TEST(Platform::IsFileExistsByFullPath(archivePath), ());
  TEST_EQUAL(ArchiveReadAllBytes(archivePath), archiveBefore, ());

  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);
}

UNIT_TEST(Archive_CorruptPixKeepsPixAndArchive)
{
  std::string const countryId = "sp018_corrupt_scan";
  std::string const pixPath = ArchiveTestPath(countryId + ".pix");
  std::string const archivePath = ArchiveTestPath(countryId + ".pixr");
  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);

  street_pixels_file::ExploredEverLiveMap preexisting;
  preexisting[77] = true;
  TEST(street_pixels_file::SaveExploredArchive(archivePath, preexisting, 2), ());
  auto const archiveBefore = ArchiveReadAllBytes(archivePath);

  ArchiveWriteRawBytes(pixPath, {0xab, 0xcd, 0xef});
  auto const pixBefore = ArchiveReadAllBytes(pixPath);
  TEST_EQUAL(static_cast<int>(street_pixels_file::ProbeFile(pixPath).kind),
             static_cast<int>(street_pixels_file::FileKind::Corrupt), ());
  TEST(!street_pixels_file::ScanExploredEverLive(pixPath).has_value(), ());

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.CleanupStreetPixelsForTesting(countryId);

  TEST(Platform::IsFileExistsByFullPath(pixPath), ());
  TEST_EQUAL(ArchiveReadAllBytes(pixPath), pixBefore, ());
  TEST(Platform::IsFileExistsByFullPath(archivePath), ());
  TEST_EQUAL(ArchiveReadAllBytes(archivePath), archiveBefore, ());

  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);
}

UNIT_TEST(Archive_ProbeRejectsArchiveMagicAsLegacy)
{
  std::string const archivePath = ArchiveTestPath("sp018_magic.pixr");
  std::string const pixPath = ArchiveTestPath("sp018_magic.pix");
  ArchiveRemoveIfExists(archivePath);
  ArchiveRemoveIfExists(pixPath);

  street_pixels_file::ExploredEverLiveMap explored;
  explored[5] = true;
  TEST(street_pixels_file::SaveExploredArchive(archivePath, explored, 3), ());

  auto const bytes = ArchiveReadAllBytes(archivePath);
  ArchiveWriteRawBytes(pixPath, bytes);

  auto const probe = street_pixels_file::ProbeFile(pixPath);
  TEST_EQUAL(static_cast<int>(probe.kind), static_cast<int>(street_pixels_file::FileKind::UnsupportedFormat), ());
  TEST_EQUAL(probe.header.magic, street_pixels_file::kArchiveMagic, ());
  TEST(!street_pixels_file::MayRecoverByDerive(probe.kind), ());
  TEST(!street_pixels_file::ScanExploredEverLive(pixPath).has_value(), ());

  ArchiveRemoveIfExists(archivePath);
  ArchiveRemoveIfExists(pixPath);
}

UNIT_TEST(Archive_RematchFallsBackToPixrWhenPixUnreadable)
{
  std::string const countryId = "sp018_fallback";
  std::string const pixPath = ArchiveTestPath(countryId + ".pix");
  std::string const archivePath = ArchiveTestPath(countryId + ".pixr");
  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);

  street_pixels_file::ExploredEverLiveMap archived;
  archived[10] = true;
  archived[20] = false;
  TEST(street_pixels_file::SaveExploredArchive(archivePath, archived, 1), ());

  auto bytes = ArchiveEncodePixHeaderBytes(99, street_pixels_file::kFlagsHasHeaderBit, 7);
  ArchiveWriteRawBytes(pixPath, bytes);
  TEST(!street_pixels_file::ScanExploredEverLive(pixPath).has_value(), ());

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  std::set<int64_t> const newIds{10, 20, 30};
  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, newIds, 44), ());

  TEST(Platform::IsFileExistsByFullPath(pixPath), ());
  TEST(!Platform::IsFileExistsByFullPath(archivePath), ());
  TEST_EQUAL(ArchiveCountExploredWords(ArchiveReadPixBodyWords(pixPath)), 2, ());
  TEST_EQUAL(street_pixels_file::ProbeFile(pixPath).header.mapDataVersion, 44, ());

  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);
}

UNIT_TEST(Archive_RedownloadRematchFromPixrOnly)
{
  std::string const countryId = "sp018_redownload";
  std::string const pixPath = ArchiveTestPath(countryId + ".pix");
  std::string const archivePath = ArchiveTestPath(countryId + ".pixr");
  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);

  street_pixels_file::ExploredEverLiveMap archived;
  archived[10] = true;
  archived[20] = false;
  archived[30] = true;
  TEST(street_pixels_file::SaveExploredArchive(archivePath, archived, 1), ());
  TEST(!Platform::IsFileExistsByFullPath(pixPath), ());

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  std::set<int64_t> const newIds{10, 20, 40};
  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, newIds, 88), ());

  TEST(Platform::IsFileExistsByFullPath(pixPath), ());
  TEST(!Platform::IsFileExistsByFullPath(archivePath), ());

  auto const words = ArchiveReadPixBodyWords(pixPath);
  TEST_EQUAL(words.size(), 3, ());
  TEST_EQUAL(ArchiveCountExploredWords(words), 2, ());

  auto const probe = street_pixels_file::ProbeFile(pixPath);
  TEST_EQUAL(probe.header.mapDataVersion, 88, ());

  bool found10 = false;
  bool found20 = false;
  bool found40 = false;
  for (int64_t word : words)
  {
    int64_t const id = word & street_pixels_file::kPixelIdMask;
    bool const explored = (word & street_pixels_file::kExploredBit) != 0;
    bool const everLive = (word & street_pixels_file::kEverLiveBit) != 0;
    if (id == 10)
    {
      found10 = true;
      TEST(explored, ());
      TEST(everLive, ());
    }
    else if (id == 20)
    {
      found20 = true;
      TEST(explored, ());
      TEST(!everLive, ());
    }
    else if (id == 40)
    {
      found40 = true;
      TEST(!explored, ());
    }
  }
  TEST(found10 && found20 && found40, ());

  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);
}

UNIT_TEST(Archive_RematchFailKeepsArchive)
{
  std::string const countryId = "sp018_rematch_fail";
  std::string const pixPath = ArchiveTestPath(countryId + ".pix");
  std::string const archivePath = ArchiveTestPath(countryId + ".pixr");
  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);

  ArchiveWriteRawBytes(archivePath, {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  auto const archiveBefore = ArchiveReadAllBytes(archivePath);
  TEST(!street_pixels_file::LoadExploredArchive(archivePath).has_value(), ());

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  std::set<int64_t> const newIds{1, 2, 3};
  TEST(!manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, newIds, 5), ());
  TEST(!Platform::IsFileExistsByFullPath(pixPath), ());
  TEST(Platform::IsFileExistsByFullPath(archivePath), ());
  TEST_EQUAL(ArchiveReadAllBytes(archivePath), archiveBefore, ());

  ArchiveRemoveIfExists(archivePath);
}

UNIT_TEST(Archive_OrphanCleanup)
{
  std::string const countryId = "sp018_orphan";
  std::string const pixPath = ArchiveTestPath(countryId + ".pix");
  std::string const archivePath = ArchiveTestPath(countryId + ".pixr");
  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);

  int64_t constexpr kVersion = 55;
  ArchiveWriteHeaderedPixWords(pixPath, kVersion, {street_pixels_file::PackPixelEntry(11, true, true), 22});
  auto const pixBefore = ArchiveReadAllBytes(pixPath);

  street_pixels_file::ExploredEverLiveMap orphan;
  orphan[99] = true;
  TEST(street_pixels_file::SaveExploredArchive(archivePath, orphan, 1), ());
  TEST(Platform::IsFileExistsByFullPath(archivePath), ());

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  std::set<int64_t> const newIds{11, 22, 33};
  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, newIds, kVersion), ());

  TEST_EQUAL(ArchiveReadAllBytes(pixPath), pixBefore, ());
  TEST(!Platform::IsFileExistsByFullPath(archivePath), ());

  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);
}

UNIT_TEST(Archive_LoadPathNoBlankDerive)
{
  std::string const countryId = "sp018_no_blank";
  std::string const pixPath = ArchiveTestPath(countryId + ".pix");
  std::string const archivePath = ArchiveTestPath(countryId + ".pixr");
  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);

  street_pixels_file::ExploredEverLiveMap archived;
  archived[42] = true;
  archived[43] = false;
  TEST(street_pixels_file::SaveExploredArchive(archivePath, archived, 2), ());

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  std::set<int64_t> const newIds{42, 43, 44};
  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, newIds, 9), ());

  TEST(Platform::IsFileExistsByFullPath(pixPath), ());
  TEST(!Platform::IsFileExistsByFullPath(archivePath), ());
  TEST_EQUAL(ArchiveCountExploredWords(ArchiveReadPixBodyWords(pixPath)), 2, ());

  auto const scanned = street_pixels_file::ScanExploredEverLive(pixPath);
  TEST(scanned.has_value(), ());
  TEST_EQUAL(scanned->size(), 2, ());
  TEST(scanned->at(42), ());
  TEST(!scanned->at(43), ());

  ArchiveRemoveIfExists(pixPath);
  ArchiveRemoveIfExists(archivePath);
}
