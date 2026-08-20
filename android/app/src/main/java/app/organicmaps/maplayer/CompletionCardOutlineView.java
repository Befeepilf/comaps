package app.organicmaps.maplayer;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.util.AttributeSet;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class CompletionCardOutlineView extends View
{
  private static final float CANONICAL_SIZE = 512f;

  @NonNull
  private final Paint mStrokePaint;
  @NonNull
  private final Path mPath;
  @Nullable
  private float[] mXs;
  @Nullable
  private float[] mYs;
  @Nullable
  private int[] mRingLengths;

  public CompletionCardOutlineView(Context context)
  {
    this(context, null);
  }

  public CompletionCardOutlineView(Context context, @Nullable AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public CompletionCardOutlineView(Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    mStrokePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    mStrokePaint.setStyle(Paint.Style.STROKE);
    mStrokePaint.setStrokeWidth(4f);
    mStrokePaint.setColor(Color.BLACK);
    mStrokePaint.setStrokeJoin(Paint.Join.ROUND);
    mStrokePaint.setStrokeCap(Paint.Cap.ROUND);
    mPath = new Path();
  }

  public void setOutline(@Nullable float[] xs, @Nullable float[] ys, @Nullable int[] ringLengths)
  {
    mXs = xs;
    mYs = ys;
    mRingLengths = ringLengths;
    rebuildPath();
    invalidate();
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh)
  {
    super.onSizeChanged(w, h, oldw, oldh);
    rebuildPath();
  }

  @Override
  protected void onDraw(@NonNull Canvas canvas)
  {
    super.onDraw(canvas);
    if (mPath.isEmpty())
      return;
    canvas.drawPath(mPath, mStrokePaint);
  }

  private void rebuildPath()
  {
    mPath.reset();
    if (mXs == null || mYs == null || mRingLengths == null)
      return;
    if (mXs.length == 0 || mXs.length != mYs.length || getWidth() <= 0 || getHeight() <= 0)
      return;
    float scaleX = getWidth() / CANONICAL_SIZE;
    float scaleY = getHeight() / CANONICAL_SIZE;
    int cursor = 0;
    for (int ringLength : mRingLengths)
    {
      if (ringLength < 3 || cursor + ringLength > mXs.length)
        return;
      mPath.moveTo(mXs[cursor] * scaleX, mYs[cursor] * scaleY);
      for (int i = 1; i < ringLength; ++i)
        mPath.lineTo(mXs[cursor + i] * scaleX, mYs[cursor + i] * scaleY);
      mPath.close();
      cursor += ringLength;
    }
  }
}
