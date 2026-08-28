package app.organicmaps.sdk.bookmarks.data;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class BookmarkManagerGpxGateTest
{
  @Test
  public void isGpxFilename_trueForGpx()
  {
    assertTrue(BookmarkManager.isGpxFilename("track.gpx"));
    assertTrue(BookmarkManager.isGpxFilename("track.GPX"));
  }

  @Test
  public void isGpxFilename_falseForKmlAndNull()
  {
    assertFalse(BookmarkManager.isGpxFilename("track.kml"));
    assertFalse(BookmarkManager.isGpxFilename("track.kmz"));
    assertFalse(BookmarkManager.isGpxFilename(null));
  }

  @Test
  public void allowGpxInBatch_unavailableNotEntitled()
  {
    assertFalse(BookmarkManager.allowGpxInBatch(1, false, false));
    assertFalse(BookmarkManager.allowGpxInBatch(2, false, false));
  }

  @Test
  public void allowGpxInBatch_unavailableEntitled()
  {
    assertFalse(BookmarkManager.allowGpxInBatch(1, false, true));
    assertFalse(BookmarkManager.allowGpxInBatch(2, false, true));
  }

  @Test
  public void allowGpxInBatch_availableNotEntitled()
  {
    assertTrue(BookmarkManager.allowGpxInBatch(1, true, false));
    assertFalse(BookmarkManager.allowGpxInBatch(2, true, false));
  }

  @Test
  public void allowGpxInBatch_availableEntitled()
  {
    assertTrue(BookmarkManager.allowGpxInBatch(1, true, true));
    assertTrue(BookmarkManager.allowGpxInBatch(2, true, true));
  }
}
