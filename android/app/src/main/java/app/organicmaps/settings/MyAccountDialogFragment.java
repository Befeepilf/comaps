package app.organicmaps.settings;

import android.app.Dialog;
import android.os.Bundle;
import android.text.InputFilter;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentManager;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.materialswitch.MaterialSwitch;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.R;

public class MyAccountDialogFragment extends DialogFragment
{
  public static final String ARG_ADD_FRIEND_USERNAME = "add_friend_username";
  public static final String RESULT_COMPETITION_ACCOUNT = "competition_account_changed";

  private MaterialSwitch mSyncSwitch;
  private TextInputLayout mUsernameLayout;
  private TextInputEditText mUsernameEdit;
  private MaterialButton mBtnSignup;
  private MaterialButton mBtnChangeUsername;
  private MaterialButton mBtnLeaveCompetition;
  private MaterialButton mBtnDeleteCompetition;

  public static void show(@NonNull FragmentManager fm)
  {
    new MyAccountDialogFragment().show(fm, "my_account_dialog");
  }

  public static void showWithAddFriend(@NonNull FragmentManager fm, @NonNull String username)
  {
    show(fm);
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
  {
    LayoutInflater inflater = LayoutInflater.from(requireContext());
    View view = inflater.inflate(R.layout.dialog_my_account, null);

    mSyncSwitch = view.findViewById(R.id.sync_switch);
    mSyncSwitch.setChecked(Framework.nativeGetExploreSyncEnabled());

    mUsernameLayout = view.findViewById(R.id.username_input_layout);
    mUsernameEdit = view.findViewById(R.id.username_edit);
    mUsernameEdit.setFilters(new InputFilter[]{ new InputFilter.LengthFilter(24) });
    mBtnSignup = view.findViewById(R.id.btn_signup);
    mBtnChangeUsername = view.findViewById(R.id.btn_change_username);
    mBtnLeaveCompetition = view.findViewById(R.id.btn_leave_competition);
    mBtnDeleteCompetition = view.findViewById(R.id.btn_delete_competition);

    prefillNickname();
    updateSignupVisibility();

    mBtnSignup.setOnClickListener(v -> claimNickname());
    mBtnChangeUsername.setOnClickListener(v -> {
      if (mUsernameEdit.isEnabled())
        claimNickname();
      else
      {
        mUsernameEdit.setEnabled(true);
        mUsernameEdit.requestFocus();
        mBtnChangeUsername.setText(R.string.save);
      }
    });
    mBtnLeaveCompetition.setOnClickListener(v -> new MaterialAlertDialogBuilder(requireContext())
        .setTitle(R.string.competition_leave_confirm_title)
        .setMessage(R.string.competition_leave_confirm_message)
        .setPositiveButton(R.string.competition_leave, (d, w) -> leaveCompetition())
        .setNegativeButton(R.string.cancel, null)
        .show());
    mBtnDeleteCompetition.setOnClickListener(v -> new MaterialAlertDialogBuilder(requireContext())
        .setTitle(R.string.competition_delete_confirm_title)
        .setMessage(R.string.competition_delete_confirm_message)
        .setPositiveButton(R.string.competition_delete, (d, w) -> deleteCompetition())
        .setNegativeButton(R.string.cancel, null)
        .show());

    return new MaterialAlertDialogBuilder(requireContext())
        .setTitle(R.string.my_account)
        .setView(view)
        .setPositiveButton(R.string.save, (d, w) -> Framework.nativeSetExploreSyncEnabled(mSyncSwitch.isChecked()))
        .setNegativeButton(R.string.cancel, null)
        .create();
  }

  private void prefillNickname()
  {
    String draft = Framework.nativeGetNicknameDraft();
    String accepted = Framework.nativeGetUsername();
    if (draft != null && !draft.isEmpty())
      mUsernameEdit.setText(draft);
    else if (accepted != null && !accepted.isEmpty())
      mUsernameEdit.setText(accepted);
    else
      mUsernameEdit.setText(Framework.nativeGenerateNickname());
  }

  private boolean ensureConsentForSignup()
  {
    if (Framework.nativeHasExploreConsent())
      return true;

    ExploreConsentDialogFragment.maybeShow(getParentFragmentManager(), new ExploreConsentDialogFragment.Listener()
    {
      @Override
      public void onExploreConsentGranted()
      {
        claimNickname();
      }

      @Override
      public void onExploreConsentDeclined()
      {
      }
    });
    return false;
  }

  private void claimNickname()
  {
    if (!ensureConsentForSignup())
      return;
    String name = mUsernameEdit.getText() == null ? "" : mUsernameEdit.getText().toString();
    if (!Framework.nativeIsValidNickname(name))
    {
      mUsernameLayout.setError(getString(R.string.friends_username_too_short));
      return;
    }
    mUsernameLayout.setError(null);
    setAccountActionsEnabled(false);
    ThreadPool.getWorker().execute(() -> {
      int result = Framework.nativeTryClaimNickname(name);
      UiThread.run(() -> {
        if (!isAdded())
          return;
        setAccountActionsEnabled(true);
        applyClaimResult(result);
      });
    });
  }

  private void applyClaimResult(int result)
  {
    if (result == 0)
    {
      Toast.makeText(requireContext(), R.string.competition_nickname_saved, Toast.LENGTH_SHORT).show();
      updateSignupVisibility();
      return;
    }
    if (result == 2)
    {
      mUsernameLayout.setError(getString(R.string.friends_signup_error));
      mUsernameEdit.setText(Framework.nativeGenerateNickname());
      return;
    }
    if (result == 3)
    {
      mUsernameLayout.setError(getString(R.string.friends_signup_error));
      if (Framework.nativeHasUsername())
        mUsernameEdit.setText(Framework.nativeGetUsername());
      return;
    }
    mUsernameLayout.setError(getString(R.string.friends_signup_error));
  }

  private void setAccountActionsEnabled(boolean enabled)
  {
    mBtnSignup.setEnabled(enabled);
    mBtnChangeUsername.setEnabled(enabled);
    mBtnLeaveCompetition.setEnabled(enabled);
    mBtnDeleteCompetition.setEnabled(enabled);
  }

  private void updateSignupVisibility()
  {
    boolean hasUsername = Framework.nativeHasUsername();
    if (hasUsername)
    {
      mUsernameEdit.setText(Framework.nativeGetUsername());
      mUsernameEdit.setEnabled(false);
      mBtnSignup.setVisibility(View.GONE);
      mBtnChangeUsername.setVisibility(View.VISIBLE);
      mBtnChangeUsername.setText(R.string.edit);
    }
    else
    {
      mUsernameEdit.setEnabled(true);
      mBtnSignup.setVisibility(View.VISIBLE);
      mBtnSignup.setEnabled(true);
      mBtnChangeUsername.setVisibility(View.GONE);
    }
    boolean showCompetitionActions = Framework.nativeHasExploreConsent() || Framework.nativeHasUsername();
    mBtnLeaveCompetition.setVisibility(showCompetitionActions ? View.VISIBLE : View.GONE);
    mBtnDeleteCompetition.setVisibility(showCompetitionActions ? View.VISIBLE : View.GONE);
  }

  private void leaveCompetition()
  {
    setAccountActionsEnabled(false);
    ThreadPool.getWorker().execute(() -> {
      Framework.nativeLeaveCompetitionRetain();
      UiThread.run(() -> {
        if (!isAdded())
          return;
        Toast.makeText(requireContext(), R.string.competition_leave_done, Toast.LENGTH_LONG).show();
        getParentFragmentManager().setFragmentResult(RESULT_COMPETITION_ACCOUNT, new Bundle());
        dismiss();
      });
    });
  }

  private void deleteCompetition()
  {
    setAccountActionsEnabled(false);
    ThreadPool.getWorker().execute(() -> {
      int result = Framework.nativeDeleteCompetitionProfile();
      UiThread.run(() -> {
        if (!isAdded())
          return;
        setAccountActionsEnabled(true);
        if (result != 0)
        {
          Toast.makeText(requireContext(), R.string.competition_delete_unavailable, Toast.LENGTH_LONG).show();
          return;
        }
        getParentFragmentManager().setFragmentResult(RESULT_COMPETITION_ACCOUNT, new Bundle());
        dismiss();
      });
    });
  }
}
