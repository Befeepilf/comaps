package app.organicmaps.sdk.maplayer.streetpixels;

import androidx.annotation.Keep;

@Keep
public class FirstGoalProgress
{
  public static final int STATE_HIDDEN = 0;
  public static final int STATE_IN_PROGRESS = 1;
  public static final int STATE_COMPLETE = 2;

  public final int state;
  public final int collected;
  public final int threshold;

  @Keep
  public FirstGoalProgress(int state, int collected, int threshold)
  {
    this.state = state;
    this.collected = collected;
    this.threshold = threshold;
  }

  public boolean isVisible()
  {
    return state == STATE_IN_PROGRESS;
  }
}
