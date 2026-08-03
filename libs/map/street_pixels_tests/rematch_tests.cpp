#include "testing/testing.hpp"

#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"
#include "map/street_stats_db.hpp"

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
#include <unordered_map>
#include <vector>

namespace
{
std::string RematchTestPixPath(std::string const & name)
{
  return base::JoinPath(GetPlatform().WritableDir(), name);
}

void RematchRemoveIfExists(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
}

void RematchWriteRawBytes(std::string const & path, std::vector<uint8_t> const & bytes)
{
  FileWriter writer(path, FileWriter::OP_WRITE_TRUNCATE);
  if (!bytes.empty())
    writer.Write(bytes.data(), bytes.size());
  writer.Flush();
}

std::vector<uint8_t> RematchReadAllBytes(std::string const & path)
{
  FileReader reader(path);
  std::vector<uint8_t> bytes(static_cast<size_t>(reader.Size()));
  if (!bytes.empty())
    reader.Read(0, bytes.data(), bytes.size());
  return bytes;
}

std::vector<uint8_t> RematchEncodeHeaderBytes(uint16_t formatVersion, uint16_t flags, int64_t mapDataVersion)
{
  std::vector<uint8_t> bytes;
  MemWriter<std::vector<uint8_t>> writer(bytes);
  street_pixels_file::WriteHeader(writer, mapDataVersion, formatVersion, flags);
  return bytes;
}

void RematchWriteHeaderedRawWords(std::string const & path, int64_t mapDataVersion, std::vector<int64_t> const & words,
                                  uint16_t formatVersion = street_pixels_file::kFormatVersionV2)
{
  auto bytes = RematchEncodeHeaderBytes(formatVersion, street_pixels_file::kFlagsHasHeaderBit, mapDataVersion);
  for (int64_t word : words)
  {
    uint8_t raw[sizeof(int64_t)];
    std::memcpy(raw, &word, sizeof(word));
    bytes.insert(bytes.end(), raw, raw + sizeof(raw));
  }
  RematchWriteRawBytes(path, bytes);
}

std::vector<int64_t> RematchReadBodyWords(std::string const & path)
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

size_t RematchCountExploredWords(std::vector<int64_t> const & words)
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

UNIT_TEST(Rematch_UnchangedRemovedAddedMatrix)
{
  std::string const path = RematchTestPixPath("sp017_matrix.pix");
  RematchRemoveIfExists(path);

  int64_t constexpr kOldVersion = 10;
  int64_t constexpr kNewVersion = 20;
  int64_t constexpr kUnchanged = 100;
  int64_t constexpr kRemoved = 200;
  int64_t constexpr kAdded = 300;

  RematchWriteHeaderedRawWords(path, kOldVersion,
                               {street_pixels_file::PackPixelEntry(kUnchanged, true, false),
                                street_pixels_file::PackPixelEntry(kRemoved, true, true),
                                street_pixels_file::PackPixelEntry(400, false, false)});

  std::set<int64_t> const newIds{kUnchanged, kAdded};
  auto const explored = street_pixels_file::ScanExploredEverLive(path);
  TEST(explored.has_value(), ());
  TEST_EQUAL(explored->size(), 2, ());
  TEST(explored->count(kUnchanged) == 1, ());
  TEST(!explored->at(kUnchanged), ());
  TEST(explored->count(kRemoved) == 1, ());
  TEST(explored->at(kRemoved), ());

  TEST(street_pixels_file::SaveRematchedUniverse(path, newIds, *explored, kNewVersion), ());

  auto const probe = street_pixels_file::ProbeFile(path);
  TEST_EQUAL(static_cast<int>(probe.kind), static_cast<int>(street_pixels_file::FileKind::HeaderedV2), ());
  TEST_EQUAL(probe.header.mapDataVersion, kNewVersion, ());
  TEST_EQUAL(probe.header.formatVersion, street_pixels_file::kFormatVersionV2, ());

  auto const words = RematchReadBodyWords(path);
  TEST_EQUAL(words.size(), 2, ());
  TEST_EQUAL(RematchCountExploredWords(words), 1, ());

  bool foundUnchanged = false;
  bool foundAdded = false;
  bool foundRemoved = false;
  for (int64_t word : words)
  {
    int64_t const id = word & street_pixels_file::kPixelIdMask;
    if (id == kUnchanged)
    {
      foundUnchanged = true;
      TEST((word & street_pixels_file::kExploredBit) != 0, ());
      TEST((word & street_pixels_file::kEverLiveBit) == 0, ());
    }
    else if (id == kAdded)
    {
      foundAdded = true;
      TEST((word & street_pixels_file::kExploredBit) == 0, ());
      TEST((word & street_pixels_file::kEverLiveBit) == 0, ());
    }
    else if (id == kRemoved)
      foundRemoved = true;
  }
  TEST(foundUnchanged, ());
  TEST(foundAdded, ());
  TEST(!foundRemoved, ());

  RematchRemoveIfExists(path);
}

UNIT_TEST(Rematch_EverLivePersistsForSurvivors)
{
  std::string const path = RematchTestPixPath("sp017_ever_live.pix");
  RematchRemoveIfExists(path);

  int64_t constexpr kLive = 111;
  int64_t constexpr kImported = 222;
  int64_t constexpr kNewVersion = 33;

  RematchWriteHeaderedRawWords(path, 1,
                               {street_pixels_file::PackPixelEntry(kLive, true, true),
                                street_pixels_file::PackPixelEntry(kImported, true, false)});

  std::set<int64_t> const newIds{kLive, kImported, 333};
  auto const explored = street_pixels_file::ScanExploredEverLive(path);
  TEST(explored.has_value(), ());
  TEST(street_pixels_file::SaveRematchedUniverse(path, newIds, *explored, kNewVersion), ());

  auto const words = RematchReadBodyWords(path);
  TEST_EQUAL(RematchCountExploredWords(words), 2, ());
  for (int64_t word : words)
  {
    int64_t const id = word & street_pixels_file::kPixelIdMask;
    if (id == kLive)
    {
      TEST((word & street_pixels_file::kExploredBit) != 0, ());
      TEST((word & street_pixels_file::kEverLiveBit) != 0, ());
    }
    else if (id == kImported)
    {
      TEST((word & street_pixels_file::kExploredBit) != 0, ());
      TEST((word & street_pixels_file::kEverLiveBit) == 0, ());
    }
    else
    {
      TEST((word & street_pixels_file::kExploredBit) == 0, ());
      TEST((word & street_pixels_file::kEverLiveBit) == 0, ());
    }
  }

  RematchRemoveIfExists(path);
}

UNIT_TEST(Rematch_InterruptBeforeRenameKeepsOld)
{
  std::string const path = RematchTestPixPath("sp017_interrupt.pix");
  std::string const tmpPath = path + ".tmp_orphan";
  RematchRemoveIfExists(path);
  RematchRemoveIfExists(tmpPath);

  RematchWriteHeaderedRawWords(path, 7, {street_pixels_file::PackPixelEntry(55, true, true), 66});
  auto const original = RematchReadAllBytes(path);

  RematchWriteHeaderedRawWords(tmpPath, 8, {street_pixels_file::PackPixelEntry(99, true, false)});
  TEST(Platform::IsFileExistsByFullPath(tmpPath), ());

  auto const afterPartialTemp = RematchReadAllBytes(path);
  TEST_EQUAL(afterPartialTemp, original, ());
  auto const probe = street_pixels_file::ProbeFile(path);
  TEST_EQUAL(probe.header.mapDataVersion, 7, ());
  TEST_EQUAL(RematchCountExploredWords(RematchReadBodyWords(path)), 1, ());

  auto const explored = street_pixels_file::ScanExploredEverLive(path);
  TEST(explored.has_value(), ());
  std::set<int64_t> const newIds{55, 77};
  TEST(street_pixels_file::SaveRematchedUniverse(path, newIds, *explored, 8), ());
  TEST_EQUAL(street_pixels_file::ProbeFile(path).header.mapDataVersion, 8, ());
  TEST_EQUAL(RematchCountExploredWords(RematchReadBodyWords(path)), 1, ());

  RematchRemoveIfExists(path);
  RematchRemoveIfExists(tmpPath);
}

UNIT_TEST(Rematch_ProcessedTracksClearedAfterRematch)
{
  std::string const countryId = "sp017_tracks";
  std::string const path = RematchTestPixPath(countryId + ".pix");
  RematchRemoveIfExists(path);
  RematchRemoveIfExists(RematchTestPixPath(countryId + ".pixa"));
  RematchRemoveIfExists(RematchTestPixPath(countryId + ".pixf"));

  int64_t constexpr kHash = 424242;
  int64_t constexpr kOldId = 10;
  int64_t constexpr kNewId = 20;

  RematchWriteHeaderedRawWords(path, 1, {street_pixels_file::PackPixelEntry(kOldId, true, true)});
  RematchWriteRawBytes(RematchTestPixPath(countryId + ".pixa"), {0x01});
  RematchWriteRawBytes(RematchTestPixPath(countryId + ".pixf"), {0x02});

  auto & db = street_stats::StreetStatsDB::Instance();
  db.MarkTrackProcessed(kHash, countryId);
  TEST(db.IsTrackProcessed(kHash, countryId), ());

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  std::set<int64_t> const newIds{kOldId, kNewId};
  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, newIds, 99), ());

  TEST(!db.IsTrackProcessed(kHash, countryId), ());

  auto const probe = street_pixels_file::ProbeFile(path);
  TEST_EQUAL(probe.header.mapDataVersion, 99, ());
  TEST_EQUAL(probe.header.formatVersion, street_pixels_file::kFormatVersionV2, ());
  TEST_EQUAL(RematchCountExploredWords(RematchReadBodyWords(path)), 1, ());
  TEST(!Platform::IsFileExistsByFullPath(RematchTestPixPath(countryId + ".pixa")), ());
  TEST(!Platform::IsFileExistsByFullPath(RematchTestPixPath(countryId + ".pixf")), ());

  manager.SetStreetPixelsForTesting(
      {street_pixels_tests::MakeStreetPixel(kOldId, true, true), street_pixels_tests::MakeStreetPixel(kNewId, false, false)});
  manager.MarkTrackPixelsForTesting({kNewId});
  TEST(manager.IsPixelExploredForTesting(kNewId), ());
  TEST(!manager.IsPixelEverLiveForTesting(kNewId), ());

  RematchRemoveIfExists(path);
  RematchRemoveIfExists(RematchTestPixPath(countryId + ".pixa"));
  RematchRemoveIfExists(RematchTestPixPath(countryId + ".pixf"));
}

UNIT_TEST(Rematch_ChunkedLargeSyntheticUniverse)
{
  std::string const path = RematchTestPixPath("sp017_large.pix");
  RematchRemoveIfExists(path);

  size_t constexpr kUniverse = 20000;
  size_t constexpr kExploredEvery = 17;
  std::vector<int64_t> oldWords;
  oldWords.reserve(kUniverse);
  street_pixels_file::ExploredEverLiveMap expected;
  for (size_t i = 0; i < kUniverse; ++i)
  {
    int64_t const id = static_cast<int64_t>(i + 1);
    bool const explored = (i % kExploredEvery) == 0;
    bool const everLive = explored && ((i / kExploredEvery) % 2 == 0);
    oldWords.push_back(street_pixels_file::PackPixelEntry(id, explored, everLive));
    if (explored)
      expected[id] = everLive;
  }
  RematchWriteHeaderedRawWords(path, 5, oldWords);

  auto const scanned = street_pixels_file::ScanExploredEverLive(path);
  TEST(scanned.has_value(), ());
  TEST_EQUAL(scanned->size(), expected.size(), ());
  for (auto const & entry : expected)
  {
    auto const it = scanned->find(entry.first);
    TEST(it != scanned->end(), ());
    TEST_EQUAL(it->second, entry.second, ());
  }

  std::set<int64_t> newIds;
  for (size_t i = 0; i < kUniverse; ++i)
  {
    if ((i % 3) != 0)
      newIds.insert(static_cast<int64_t>(i + 1));
  }
  newIds.insert(static_cast<int64_t>(kUniverse + 100));

  TEST(street_pixels_file::SaveRematchedUniverse(path, newIds, *scanned, 6), ());

  auto const words = RematchReadBodyWords(path);
  TEST_EQUAL(words.size(), newIds.size(), ());
  size_t exploredCount = 0;
  for (int64_t word : words)
  {
    int64_t const id = word & street_pixels_file::kPixelIdMask;
    bool const explored = (word & street_pixels_file::kExploredBit) != 0;
    bool const everLive = (word & street_pixels_file::kEverLiveBit) != 0;
    auto const it = expected.find(id);
    if (it == expected.end())
    {
      TEST(!explored, ());
      TEST(!everLive, ());
    }
    else
    {
      TEST(explored, ());
      TEST_EQUAL(everLive, it->second, ());
      ++exploredCount;
    }
  }
  size_t expectedIntersection = 0;
  for (int64_t id : newIds)
  {
    if (expected.count(id) != 0)
      ++expectedIntersection;
  }
  TEST_EQUAL(exploredCount, expectedIntersection, ());

  RematchRemoveIfExists(path);
}

UNIT_TEST(Rematch_ReconcileStatsClearsProcessedTracks)
{
  std::string const countryId = "sp017_reconcile";
  auto & db = street_stats::StreetStatsDB::Instance();
  int64_t constexpr kHash = 777001;
  db.MarkTrackProcessed(kHash, countryId);
  TEST(db.IsTrackProcessed(kHash, countryId), ());
  db.ReconcileStatsAfterRematch(countryId);
  TEST(!db.IsTrackProcessed(kHash, countryId), ());
}

UNIT_TEST(Rematch_ScanFailureDoesNotWipeExplored)
{
  std::string const countryId = "sp017_scan_fail";
  std::string const path = RematchTestPixPath(countryId + ".pix");
  RematchRemoveIfExists(path);

  auto bytes = RematchEncodeHeaderBytes(99, street_pixels_file::kFlagsHasHeaderBit, 7);
  int64_t const word = street_pixels_file::PackPixelEntry(55, true, true);
  uint8_t raw[sizeof(int64_t)];
  std::memcpy(raw, &word, sizeof(word));
  bytes.insert(bytes.end(), raw, raw + sizeof(raw));
  RematchWriteRawBytes(path, bytes);

  auto const original = RematchReadAllBytes(path);
  auto const scanned = street_pixels_file::ScanExploredEverLive(path);
  TEST(!scanned.has_value(), ());

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  std::set<int64_t> const newIds{55, 77};
  TEST(!manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, newIds, 8), ());
  TEST_EQUAL(RematchReadAllBytes(path), original, ());
  TEST_EQUAL(street_pixels_file::ProbeFile(path).header.mapDataVersion, 7, ());

  RematchRemoveIfExists(path);
}

UNIT_TEST(Rematch_EqualVersionDoesNotRewrite)
{
  std::string const countryId = "sp017_equal_ver";
  std::string const path = RematchTestPixPath(countryId + ".pix");
  RematchRemoveIfExists(path);

  int64_t constexpr kVersion = 42;
  RematchWriteHeaderedRawWords(path, kVersion, {street_pixels_file::PackPixelEntry(11, true, true), 22});
  auto const original = RematchReadAllBytes(path);

  auto const probe = street_pixels_file::ProbeFile(path);
  TEST_EQUAL(probe.header.mapDataVersion, kVersion, ());
  TEST(probe.kind == street_pixels_file::FileKind::HeaderedV2, ());

  auto const scanned = street_pixels_file::ScanExploredEverLive(path);
  TEST(scanned.has_value(), ());
  TEST_EQUAL(scanned->size(), 1, ());
  TEST(scanned->at(11), ());
  TEST_EQUAL(RematchReadAllBytes(path), original, ());

  RematchRemoveIfExists(path);
}

UNIT_TEST(Rematch_DenominatorGrowsFractionDrops)
{
  std::string const countryId = "sp021_denom_grow";
  std::string const path = RematchTestPixPath(countryId + ".pix");
  RematchRemoveIfExists(path);

  RematchWriteHeaderedRawWords(path, 1,
                               {street_pixels_file::PackPixelEntry(1, true, false),
                                street_pixels_file::PackPixelEntry(2, true, false)});

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(!manager.TakePendingRematchFractionChange().has_value(), ());

  std::set<int64_t> const newIds{1, 2, 3, 4};
  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, newIds, 2), ());

  manager.LoadStreetPixelsFromFile(countryId, 2);
  TEST_EQUAL(manager.GetTotalExploredFraction(), 0.5, ());

  auto const words = RematchReadBodyWords(path);
  TEST_EQUAL(words.size(), 4, ());
  TEST_EQUAL(RematchCountExploredWords(words), 2, ());

  auto const pending = manager.TakePendingRematchFractionChange();
  TEST(pending.has_value(), ());
  TEST_EQUAL(pending->countryId, countryId, ());
  TEST_EQUAL(pending->previousTotal, 2, ());
  TEST_EQUAL(pending->previousExplored, 2, ());
  TEST_EQUAL(pending->newTotal, 4, ());
  TEST_EQUAL(pending->newExplored, 2, ());
  TEST_EQUAL(pending->previousFraction, 1.0, ());
  TEST_EQUAL(pending->newFraction, 0.5, ());
  TEST(pending->decreasedDueToUniverseGrowth, ());
  TEST(!manager.TakePendingRematchFractionChange().has_value(), ());

  RematchRemoveIfExists(path);
}

UNIT_TEST(Rematch_PreviousVsNewFractionSignal)
{
  std::string const countryId = "sp021_prev_new";
  std::string const path = RematchTestPixPath(countryId + ".pix");
  RematchRemoveIfExists(path);

  RematchWriteHeaderedRawWords(path, 3,
                               {street_pixels_file::PackPixelEntry(10, true, true),
                                street_pixels_file::PackPixelEntry(20, true, false),
                                30});

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  std::set<int64_t> const newIds{10, 20, 30, 40, 50};
  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, newIds, 4), ());

  auto const pending = manager.TakePendingRematchFractionChange();
  TEST(pending.has_value(), ());
  TEST_EQUAL(pending->previousTotal, 3, ());
  TEST_EQUAL(pending->previousExplored, 2, ());
  TEST_EQUAL(pending->newTotal, 5, ());
  TEST_EQUAL(pending->newExplored, 2, ());
  TEST(pending->previousFraction > pending->newFraction, ());
  TEST(pending->decreasedDueToUniverseGrowth, ());

  manager.LoadStreetPixelsFromFile(countryId, 4);
  double const expected = 2.0 / 5.0;
  TEST_EQUAL(manager.GetTotalExploredFraction(), expected, ());

  RematchRemoveIfExists(path);
}

UNIT_TEST(Rematch_NoFractionDropLeavesNoPending)
{
  std::string const countryId = "sp021_no_drop";
  std::string const path = RematchTestPixPath(countryId + ".pix");
  RematchRemoveIfExists(path);

  RematchWriteHeaderedRawWords(path, 1,
                               {street_pixels_file::PackPixelEntry(1, true, false), 2, 3, 4});

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  std::set<int64_t> const newIds{1, 2};
  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, newIds, 2), ());
  TEST(!manager.TakePendingRematchFractionChange().has_value(), ());

  RematchRemoveIfExists(path);
}

UNIT_TEST(Rematch_FailLeavesNoPending)
{
  std::string const countryId = "sp021_fail";
  std::string const path = RematchTestPixPath(countryId + ".pix");
  RematchRemoveIfExists(path);

  auto bytes = RematchEncodeHeaderBytes(99, street_pixels_file::kFlagsHasHeaderBit, 7);
  int64_t const word = street_pixels_file::PackPixelEntry(55, true, true);
  uint8_t raw[sizeof(int64_t)];
  std::memcpy(raw, &word, sizeof(word));
  bytes.insert(bytes.end(), raw, raw + sizeof(raw));
  RematchWriteRawBytes(path, bytes);

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  std::set<int64_t> const newIds{55, 77};
  TEST(!manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, newIds, 8), ());
  TEST(!manager.TakePendingRematchFractionChange().has_value(), ());

  RematchRemoveIfExists(path);
}

UNIT_TEST(Rematch_EqualVersionLeavesNoPending)
{
  std::string const countryId = "sp021_equal_ver";
  std::string const path = RematchTestPixPath(countryId + ".pix");
  RematchRemoveIfExists(path);

  int64_t constexpr kVersion = 11;
  RematchWriteHeaderedRawWords(path, kVersion,
                               {street_pixels_file::PackPixelEntry(1, true, false),
                                street_pixels_file::PackPixelEntry(2, true, false)});

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  std::set<int64_t> const newIds{1, 2, 3, 4};
  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, newIds, kVersion), ());
  TEST(!manager.TakePendingRematchFractionChange().has_value(), ());
  TEST_EQUAL(RematchReadBodyWords(path).size(), 2, ());

  RematchRemoveIfExists(path);
}

UNIT_TEST(Rematch_WrongCountryTakeLeavesPending)
{
  std::string const countryId = "sp021_wrong_country";
  std::string const path = RematchTestPixPath(countryId + ".pix");
  RematchRemoveIfExists(path);

  RematchWriteHeaderedRawWords(path, 1,
                               {street_pixels_file::PackPixelEntry(1, true, false),
                                street_pixels_file::PackPixelEntry(2, true, false)});

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  std::set<int64_t> const newIds{1, 2, 3, 4};
  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, newIds, 2), ());

  TEST(!manager.TakePendingRematchFractionChange("other_country").has_value(), ());
  auto const pending = manager.TakePendingRematchFractionChange(countryId);
  TEST(pending.has_value(), ());
  TEST_EQUAL(pending->countryId, countryId, ());
  TEST(pending->decreasedDueToUniverseGrowth, ());
  TEST(!manager.TakePendingRematchFractionChange(countryId).has_value(), ());

  RematchRemoveIfExists(path);
}

UNIT_TEST(Rematch_SuccessfulNonDropClearsSameCountryPending)
{
  std::string const countryId = "sp021_clear_pending";
  std::string const path = RematchTestPixPath(countryId + ".pix");
  RematchRemoveIfExists(path);

  RematchWriteHeaderedRawWords(path, 1,
                               {street_pixels_file::PackPixelEntry(1, true, false),
                                street_pixels_file::PackPixelEntry(2, true, false)});

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, {1, 2, 3, 4}, 2), ());
  TEST(manager.TakePendingRematchFractionChange(countryId).has_value(), ());

  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, {1, 2, 3, 4, 5, 6, 7, 8}, 3), ());
  TEST(!manager.TakePendingRematchFractionChange("other_country").has_value(), ());

  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(countryId, {1, 2}, 4), ());
  TEST(!manager.TakePendingRematchFractionChange(countryId).has_value(), ());

  RematchRemoveIfExists(path);
}
