package app.organicmaps.maplayer.streetpixels;

import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import app.organicmaps.R;
import app.organicmaps.sdk.maplayer.streetpixels.FocusedAreaProgress;
import app.organicmaps.util.UiUtils;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.textview.MaterialTextView;
import java.util.Objects;

public class FocusedAreaDetailBottomSheet extends BottomSheetDialogFragment
{
  private static final String TAG = "FocusedAreaDetailBottomSheet";
  private static final String ARG_NAME = "name";
  private static final String ARG_FRACTION = "fraction";
  private static final String ARG_FRACTION_VALID = "fraction_valid";
  private static final String ARG_AREA_COMPLETED = "area_completed";
  private static final String ARG_PREVIOUSLY_COMPLETED = "previously_completed";
  private static final String ARG_EMPTY = "empty";

  public static void show(@NonNull FragmentManager fm, @NonNull String displayName, boolean fractionValid,
                          double fraction, boolean areaCompleted, boolean previouslyCompleted)
  {
    FocusedAreaDetailBottomSheet sheet = new FocusedAreaDetailBottomSheet();
    Bundle args = new Bundle();
    args.putString(ARG_NAME, displayName);
    args.putBoolean(ARG_FRACTION_VALID, fractionValid);
    args.putDouble(ARG_FRACTION, fraction);
    args.putBoolean(ARG_AREA_COMPLETED, areaCompleted);
    args.putBoolean(ARG_PREVIOUSLY_COMPLETED, previouslyCompleted);
    args.putBoolean(ARG_EMPTY, false);
    sheet.setArguments(args);
    dismissIfShowing(fm);
    sheet.show(fm, TAG);
  }

  public static void showEmpty(@NonNull FragmentManager fm)
  {
    FocusedAreaDetailBottomSheet sheet = new FocusedAreaDetailBottomSheet();
    Bundle args = new Bundle();
    args.putBoolean(ARG_EMPTY, true);
    sheet.setArguments(args);
    dismissIfShowing(fm);
    sheet.show(fm, TAG);
  }

  public static void dismissIfShowing(@NonNull FragmentManager fm)
  {
    Fragment existing = fm.findFragmentByTag(TAG);
    if (existing instanceof FocusedAreaDetailBottomSheet)
      ((FocusedAreaDetailBottomSheet) existing).dismissAllowingStateLoss();
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
  {
    return new BottomSheetDialog(requireContext(), R.style.MwmTheme_BottomSheetDialog) {
      @Override
      public void onAttachedToWindow()
      {
        super.onAttachedToWindow();
        Window window = Objects.requireNonNull(getWindow());
        WindowInsetsControllerCompat insetsController = WindowCompat.getInsetsController(window, window.getDecorView());
        insetsController.setAppearanceLightNavigationBars(false);
      }
    };
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.focused_area_detail_bottom_sheet, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    Bundle args = requireArguments();
    MaterialTextView nameView = view.findViewById(R.id.focused_area_detail_name);
    MaterialTextView percentView = view.findViewById(R.id.focused_area_detail_percent);
    MaterialTextView bodyView = view.findViewById(R.id.focused_area_detail_body);

    if (args.getBoolean(ARG_EMPTY, false))
    {
      nameView.setText(R.string.street_pixels_no_exploration_area_title);
      percentView.setText("");
      bodyView.setText(R.string.street_pixels_no_exploration_area_message);
      UiUtils.show(bodyView);
      return;
    }

    UiUtils.hide(bodyView);
    nameView.setText(args.getString(ARG_NAME, ""));
    boolean areaCompleted = args.getBoolean(ARG_AREA_COMPLETED, false);
    if (args.getBoolean(ARG_FRACTION_VALID, false))
    {
      if (areaCompleted)
        percentView.setText(R.string.street_pixels_area_completed);
      else
        percentView.setText(FocusedAreaProgress.formatPercent(args.getDouble(ARG_FRACTION, 0.0)));
    }
    else
      percentView.setText("");
    if (args.getBoolean(ARG_PREVIOUSLY_COMPLETED, false) && !areaCompleted)
    {
      bodyView.setText(R.string.street_pixels_area_previously_completed);
      UiUtils.show(bodyView);
    }
  }
}
