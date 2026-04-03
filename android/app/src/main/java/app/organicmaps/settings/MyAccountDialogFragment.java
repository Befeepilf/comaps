package app.organicmaps.settings;

import android.app.Dialog;
import android.os.Bundle;
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
import app.organicmaps.sdk.friends.Friends;
import app.organicmaps.R;

public class MyAccountDialogFragment extends DialogFragment implements Friends.Callback
{
  private MaterialSwitch mShareSwitch;
  private TextInputLayout mUsernameLayout;
  private TextInputEditText mUsernameEdit;
  private MaterialButton mBtnSignup;
  private MaterialButton mBtnChangeUsername;
  private View mFriendsSection;
  private TextInputEditText mInputSearch;
  private View mBtnSearch;
  private android.widget.TextView mTvAccepted;
  private android.widget.TextView mTvIncoming;
  private android.widget.TextView mTvOutgoing;
  private android.widget.TextView mTvSearchResults;

  public static void show(@NonNull FragmentManager fm)
  {
    new MyAccountDialogFragment().show(fm, "my_account_dialog");
  }

  @Override
  public void onStart()
  {
    super.onStart();
    Friends.registerCallback(this);
    Friends.nativeRefresh();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    Friends.unregisterCallback(this);
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
  {
    LayoutInflater inflater = LayoutInflater.from(requireContext());
    View view = inflater.inflate(R.layout.dialog_my_account, null);

    mShareSwitch = view.findViewById(R.id.share_switch);
    mShareSwitch.setChecked(Framework.nativeGetExploreSharingEnabled());
    
    mUsernameLayout = view.findViewById(R.id.username_input_layout);
    mUsernameEdit = view.findViewById(R.id.username_edit);
    mBtnSignup = view.findViewById(R.id.btn_signup);
    mBtnChangeUsername = view.findViewById(R.id.btn_change_username);

    updateSignupVisibility();

    mBtnSignup.setOnClickListener(v -> {
      String name = mUsernameEdit.getText().toString().trim();
      if (name.length() < 3)
      {
        mUsernameLayout.setError(getString(R.string.friends_username_too_short));
        return;
      }
      mUsernameLayout.setError(null);
      mBtnSignup.setEnabled(false);
      Friends.nativeSignup(name);
    });

    mBtnChangeUsername.setOnClickListener(v -> {
      if (mUsernameEdit.isEnabled())
      {
        String name = mUsernameEdit.getText().toString().trim();
        if (name.length() < 3)
        {
          mUsernameLayout.setError(getString(R.string.friends_username_too_short));
          return;
        }
        if (name.equals(Framework.nativeGetUsername()))
        {
          mUsernameEdit.setEnabled(false);
          mBtnChangeUsername.setText(R.string.edit);
          return;
        }
        mUsernameLayout.setError(null);
        mBtnChangeUsername.setEnabled(false);
        Friends.nativeChangeUsername(name);
      }
      else
      {
        mUsernameEdit.setEnabled(true);
        mUsernameEdit.requestFocus();
        mBtnChangeUsername.setText(R.string.save);
      }
    });

    // Friends section
    mFriendsSection = view.findViewById(R.id.friends_section);
    mInputSearch = view.findViewById(R.id.input_search);
    mBtnSearch = view.findViewById(R.id.btn_search);
    mTvAccepted = view.findViewById(R.id.tv_accepted);
    mTvIncoming = view.findViewById(R.id.tv_incoming);
    mTvOutgoing = view.findViewById(R.id.tv_outgoing);
    mTvSearchResults = view.findViewById(R.id.tv_search_results);

    mFriendsSection.setEnabled(mShareSwitch.isChecked());
    mFriendsSection.setAlpha(mShareSwitch.isChecked() ? 1f : 0.5f);
    mShareSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
      mFriendsSection.setEnabled(isChecked);
      mFriendsSection.setAlpha(isChecked ? 1f : 0.5f);
    });

    mBtnSearch.setOnClickListener(v -> {
      String q = mInputSearch.getText().toString().trim();
      if (q.isEmpty()) return;
      mBtnSearch.setEnabled(false);
      Friends.nativeSearchByUsername(q, results -> {
        if (!isAdded()) return;
        mBtnSearch.setEnabled(true);
        StringBuilder sb = new StringBuilder();
        if (results != null && results.length > 0)
        {
          for (Friends.Friend f : results)
            sb.append(f.username).append(" (id:").append(f.userId).append(")\n");
        }
        else
        {
          sb.append(getString(R.string.friends_no_search_results));
        }
        mTvSearchResults.setText(sb.toString());
      });
    });

    updateLists();

    return new MaterialAlertDialogBuilder(requireContext())
        .setTitle(R.string.my_account)
        .setView(view)
        .setPositiveButton(R.string.save, (d, w) -> {
          Framework.nativeSetExploreSharingEnabled(mShareSwitch.isChecked());
        })
        .setNegativeButton(R.string.cancel, null)
        .create();
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
  }

  private void updateLists()
  {
    mTvSearchResults.setText("");
    Friends.FriendsPayload lists = Friends.nativeGetLists();
    if (lists != null)
    {
      StringBuilder a = new StringBuilder();
      if (lists.accepted != null && lists.accepted.length > 0)
        for (Friends.Friend f : lists.accepted) a.append(f.username).append("\n");
      else a.append("None");
      mTvAccepted.setText(a.toString());

      StringBuilder i = new StringBuilder();
      if (lists.incoming != null && lists.incoming.length > 0)
        for (Friends.Friend f : lists.incoming) i.append(f.username).append("\n");
      else i.append("None");
      mTvIncoming.setText(i.toString());

      StringBuilder o = new StringBuilder();
      if (lists.outgoing != null && lists.outgoing.length > 0)
        for (Friends.Friend f : lists.outgoing) o.append(f.username).append("\n");
      else o.append("None");
      mTvOutgoing.setText(o.toString());
    }
  }

  @Override
  public void onListsUpdated()
  {
    if (isAdded())
      updateLists();
  }

  @Override
  public void onSignupResult(boolean success)
  {
    if (!isAdded()) return;
    mBtnSignup.setEnabled(true);
    if (success)
    {
      Toast.makeText(requireContext(), R.string.friends_signup_success, Toast.LENGTH_SHORT).show();
      updateSignupVisibility();
      mUsernameLayout.setError(null);
    }
    else
    {
      mUsernameLayout.setError(getString(R.string.friends_signup_error));
    }
  }

  @Override
  public void onUsernameChanged(boolean success)
  {
    if (!isAdded()) return;
    mBtnChangeUsername.setEnabled(true);
    if (success)
    {
      Toast.makeText(requireContext(), R.string.friends_username_updated, Toast.LENGTH_SHORT).show();
      updateSignupVisibility();
      mUsernameLayout.setError(null);
    }
    else
    {
      mUsernameLayout.setError(getString(R.string.friends_signup_error));
    }
  }

  @Override
  public void onActionResult(boolean success)
  {
    if (!isAdded()) return;
    if (success)
      Friends.nativeRefresh();
    else
      Toast.makeText(requireContext(), R.string.friends_action_failed, Toast.LENGTH_SHORT).show();
  }
}
