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
import androidx.fragment.app.FragmentManager;
import app.organicmaps.R;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.textview.MaterialTextView;
import java.util.Locale;
import java.util.Objects;

public class FocusedAreaDetailBottomSheet extends BottomSheetDialogFragment
{
  private static final String TAG = "FocusedAreaDetailBottomSheet";
  private static final String ARG_NAME = "name";
  private static final String ARG_FRACTION = "fraction";
  private static final String ARG_FRACTION_VALID = "fraction_valid";

  public static void show(@NonNull FragmentManager fm, @NonNull String displayName, boolean fractionValid,
                          double fraction)
  {
    FocusedAreaDetailBottomSheet sheet = new FocusedAreaDetailBottomSheet();
    Bundle args = new Bundle();
    args.putString(ARG_NAME, displayName);
    args.putBoolean(ARG_FRACTION_VALID, fractionValid);
    args.putDouble(ARG_FRACTION, fraction);
    sheet.setArguments(args);
    sheet.show(fm, TAG);
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
    nameView.setText(args.getString(ARG_NAME, ""));
    if (args.getBoolean(ARG_FRACTION_VALID, false))
    {
      double percent = args.getDouble(ARG_FRACTION, 0.0) * 100.0;
      percentView.setText(String.format(Locale.US, "%.4f%%", percent));
    }
    else
      percentView.setText("");
  }
}
