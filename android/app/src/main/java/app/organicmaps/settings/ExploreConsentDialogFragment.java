package app.organicmaps.settings;

import android.app.Dialog;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentManager;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import app.organicmaps.R;
import app.organicmaps.sdk.Framework;

public class ExploreConsentDialogFragment extends DialogFragment
{
  public interface Listener
  {
    void onExploreConsentGranted();
    void onExploreConsentDeclined();
  }

  private static final String TAG = "explore_consent_dialog";

  public static boolean maybeShow(@NonNull FragmentManager fm, @NonNull Listener listener)
  {
    if (Framework.nativeHasExploreConsent())
      return false;

    ExploreConsentDialogFragment dialog = new ExploreConsentDialogFragment();
    dialog.mListener = listener;
    dialog.show(fm, TAG);
    return true;
  }

  private Listener mListener;

  @NonNull
  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
  {
    return new MaterialAlertDialogBuilder(requireContext())
        .setTitle(R.string.explore_consent_title)
        .setMessage(R.string.explore_consent_message)
        .setPositiveButton(R.string.explore_consent_accept, (d, w) -> {
          Framework.nativeSetExploreConsent(true);
          if (mListener != null)
            mListener.onExploreConsentGranted();
        })
        .setNegativeButton(R.string.explore_consent_decline, (d, w) -> {
          Framework.nativeSetExploreConsent(false);
          if (mListener != null)
            mListener.onExploreConsentDeclined();
        })
        .setCancelable(false)
        .create();
  }
}
