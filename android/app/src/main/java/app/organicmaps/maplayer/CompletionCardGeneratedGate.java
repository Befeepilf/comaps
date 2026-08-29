package app.organicmaps.maplayer;

import app.organicmaps.sdk.maplayer.streetpixels.AreaMilestonePresentation;

public final class CompletionCardGeneratedGate
{
  private CompletionCardGeneratedGate() {}

  public static boolean shouldRecord(boolean debugPreview, int threshold, long osmId, long lastRecordedOsmId)
  {
    if (debugPreview || threshold != AreaMilestonePresentation.THRESHOLD_100 || osmId == 0)
      return false;
    return osmId != lastRecordedOsmId;
  }
}
