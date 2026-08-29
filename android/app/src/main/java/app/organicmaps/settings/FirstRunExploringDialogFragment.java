package app.organicmaps.settings;

import android.app.Dialog;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentManager;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import app.organicmaps.R;
import app.organicmaps.sdk.util.Config;

public class FirstRunExploringDialogFragment extends DialogFragment
{
  public static final String RESULT_START_EXPLORING = "first_run_start_exploring";
  private static final String TAG = "first_run_exploring_dialog";

  public static boolean maybeShow(@NonNull FragmentManager fm)
  {
    if (!FirstRunFlow.shouldShowExploringCard(Config.isFirstRunExploringCardSeen()))
      return false;
    if (fm.findFragmentByTag(TAG) != null)
      return true;

    new FirstRunExploringDialogFragment().show(fm, TAG);
    return true;
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
  {
    return new MaterialAlertDialogBuilder(requireContext())
        .setTitle(R.string.street_pixels_first_run_title)
        .setMessage(R.string.street_pixels_first_run_message)
        .setPositiveButton(R.string.street_pixels_first_run_start, (d, w) -> {
          Config.setFirstRunExploringCardSeen();
          getParentFragmentManager().setFragmentResult(RESULT_START_EXPLORING, new Bundle());
        })
        .setNegativeButton(R.string.close, (d, w) -> Config.setFirstRunExploringCardSeen())
        .setCancelable(true)
        .setOnCancelListener(d -> Config.setFirstRunExploringCardSeen())
        .create();
  }
}
