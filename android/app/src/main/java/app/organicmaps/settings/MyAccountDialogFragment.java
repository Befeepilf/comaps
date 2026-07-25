package app.organicmaps.settings;

import android.app.Dialog;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;
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
  public static final String ARG_ADD_FRIEND_USERNAME = "add_friend_username";

  private MaterialSwitch mSyncSwitch;
  private MaterialSwitch mFriendVisibilitySwitch;
  private TextInputLayout mUsernameLayout;
  private TextInputEditText mUsernameEdit;
  private MaterialButton mBtnSignup;
  private MaterialButton mBtnChangeUsername;
  private View mFriendsSection;
  private TextInputEditText mInputSearch;
  private View mBtnSearch;
  private LinearLayout mLlAccepted;
  private LinearLayout mLlIncoming;
  private LinearLayout mLlOutgoing;
  private LinearLayout mLlSearchResults;
  private MaterialButton mBtnExportAccount;
  private MaterialButton mBtnDeleteAccount;

  @Nullable
  private String mPendingAddFriendUsername;

  public static void show(@NonNull FragmentManager fm)
  {
    new MyAccountDialogFragment().show(fm, "my_account_dialog");
  }

  public static void showWithAddFriend(@NonNull FragmentManager fm, @NonNull String username)
  {
    MyAccountDialogFragment fragment = new MyAccountDialogFragment();
    Bundle args = new Bundle();
    args.putString(ARG_ADD_FRIEND_USERNAME, username);
    fragment.setArguments(args);
    fragment.show(fm, "my_account_dialog");
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle args = getArguments();
    if (args != null)
      mPendingAddFriendUsername = args.getString(ARG_ADD_FRIEND_USERNAME);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    Friends.registerCallback(this);
    if (Framework.nativeHasUsername())
      Friends.nativeRefresh();
    maybeHandlePendingAddFriend();
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

    mSyncSwitch = view.findViewById(R.id.sync_switch);
    mFriendVisibilitySwitch = view.findViewById(R.id.friend_visibility_switch);
    mSyncSwitch.setChecked(Framework.nativeGetExploreSyncEnabled());
    mFriendVisibilitySwitch.setChecked(Framework.nativeGetExploreFriendVisibilityEnabled());

    mUsernameLayout = view.findViewById(R.id.username_input_layout);
    mUsernameEdit = view.findViewById(R.id.username_edit);
    mBtnSignup = view.findViewById(R.id.btn_signup);
    mBtnChangeUsername = view.findViewById(R.id.btn_change_username);

    updateSignupVisibility();

    mBtnSignup.setOnClickListener(v -> {
      if (!ensureConsentForSignup())
        return;
      String name = mUsernameEdit.getText().toString().trim().toLowerCase();
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
        String name = mUsernameEdit.getText().toString().trim().toLowerCase();
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

    mFriendsSection = view.findViewById(R.id.friends_section);
    mInputSearch = view.findViewById(R.id.input_search);
    mBtnSearch = view.findViewById(R.id.btn_search);
    mLlAccepted = view.findViewById(R.id.ll_accepted);
    mLlIncoming = view.findViewById(R.id.ll_incoming);
    mLlOutgoing = view.findViewById(R.id.ll_outgoing);
    mLlSearchResults = view.findViewById(R.id.ll_search_results);
    mBtnExportAccount = view.findViewById(R.id.btn_export_account);
    mBtnDeleteAccount = view.findViewById(R.id.btn_delete_account);

    updateFriendsSectionState();

    Runnable syncFriendVisibility = () -> {
      boolean enabled = mSyncSwitch.isChecked() && Framework.nativeHasUsername();
      mFriendVisibilitySwitch.setEnabled(enabled);
      mFriendVisibilitySwitch.setAlpha(enabled ? 1f : 0.5f);
      updateFriendsSectionState();
    };

    mSyncSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> syncFriendVisibility.run());
    syncFriendVisibility.run();

    mFriendVisibilitySwitch.setOnCheckedChangeListener((buttonView, isChecked) -> updateFriendsSectionState());

    mBtnSearch.setOnClickListener(v -> {
      if (!Framework.nativeHasUsername())
      {
        Toast.makeText(requireContext(), R.string.friends_account_required, Toast.LENGTH_SHORT).show();
        return;
      }
      String q = mInputSearch.getText().toString().trim();
      if (q.isEmpty())
        return;
      mBtnSearch.setEnabled(false);
      Friends.nativeSearchByUsername(q, results -> {
        if (!isAdded())
          return;
        mBtnSearch.setEnabled(true);
        populateFriendRows(mLlSearchResults, results, FriendRowAction.ADD);
      });
    });

    mBtnExportAccount.setOnClickListener(v -> {
      if (!Framework.nativeHasUsername())
      {
        Toast.makeText(requireContext(), R.string.friends_account_required, Toast.LENGTH_SHORT).show();
        return;
      }
      mBtnExportAccount.setEnabled(false);
      Friends.nativeExportAccount((success, json) -> {
        if (!isAdded())
          return;
        mBtnExportAccount.setEnabled(true);
        if (!success || json == null || json.isEmpty())
        {
          Toast.makeText(requireContext(), R.string.account_export_failed, Toast.LENGTH_SHORT).show();
          return;
        }
        Intent share = new Intent(Intent.ACTION_SEND);
        share.setType("application/json");
        share.putExtra(Intent.EXTRA_TEXT, json);
        share.putExtra(Intent.EXTRA_SUBJECT, getString(R.string.account_export_data));
        startActivity(Intent.createChooser(share, getString(R.string.account_export_data)));
      });
    });

    mBtnDeleteAccount.setOnClickListener(v -> new MaterialAlertDialogBuilder(requireContext())
        .setTitle(R.string.account_delete)
        .setMessage(R.string.account_delete_confirm)
        .setPositiveButton(R.string.delete, (d, w) -> {
          mBtnDeleteAccount.setEnabled(false);
          Friends.nativeDeleteAccount();
        })
        .setNegativeButton(R.string.cancel, null)
        .show());

    updateLists();

    return new MaterialAlertDialogBuilder(requireContext())
        .setTitle(R.string.my_account)
        .setView(view)
        .setPositiveButton(R.string.save, (d, w) -> {
          Framework.nativeSetExploreSyncEnabled(mSyncSwitch.isChecked());
          Framework.nativeSetExploreFriendVisibilityEnabled(mFriendVisibilitySwitch.isChecked());
        })
        .setNegativeButton(R.string.cancel, null)
        .create();
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
      }

      @Override
      public void onExploreConsentDeclined()
      {
      }
    });
    return Framework.nativeHasExploreConsent();
  }

  private void updateFriendsSectionState()
  {
    boolean enabled = Framework.nativeHasUsername()
        && mSyncSwitch.isChecked()
        && mFriendVisibilitySwitch.isChecked();
    mFriendsSection.setEnabled(enabled);
    mFriendsSection.setAlpha(enabled ? 1f : 0.5f);
    mBtnExportAccount.setEnabled(Framework.nativeHasUsername());
    mBtnDeleteAccount.setEnabled(Framework.nativeHasUsername());
  }

  private enum FriendRowAction
  {
    ADD,
    ACCEPT,
    CANCEL_OUTGOING,
    REMOVE
  }

  private void populateFriendRows(@NonNull LinearLayout container, @Nullable Friends.Friend[] friends,
                                  @NonNull FriendRowAction action)
  {
    container.removeAllViews();
    if (friends == null || friends.length == 0)
    {
      TextView empty = new TextView(requireContext());
      empty.setText(R.string.friends_none);
      container.addView(empty);
      return;
    }

    LayoutInflater inflater = LayoutInflater.from(requireContext());
    for (Friends.Friend friend : friends)
    {
      View row = inflater.inflate(R.layout.item_friend_row, container, false);
      TextView username = row.findViewById(R.id.tv_username);
      MaterialButton button = row.findViewById(R.id.btn_action);
      username.setText(friend.username);
      switch (action)
      {
      case ADD -> {
        button.setText(R.string.friends_add);
        button.setOnClickListener(v -> Friends.nativeSendRequest(friend.userId));
      }
      case ACCEPT -> {
        button.setText(R.string.friends_accept);
        button.setOnClickListener(v -> Friends.nativeAcceptRequest(friend.userId));
      }
      case CANCEL_OUTGOING -> {
        button.setText(R.string.friends_cancel);
        button.setOnClickListener(v -> Friends.nativeCancelRequest(friend.userId));
      }
      case REMOVE -> {
        button.setText(R.string.friends_remove);
        button.setOnClickListener(v -> Friends.nativeCancelRequest(friend.userId));
      }
      }
      container.addView(row);
    }
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
    updateFriendsSectionState();
  }

  private void updateLists()
  {
    mLlSearchResults.removeAllViews();
    Friends.FriendsPayload lists = Friends.nativeGetLists();
    if (lists == null)
      return;

    populateFriendRows(mLlAccepted, lists.accepted, FriendRowAction.REMOVE);
    populateIncomingRows(lists.incoming);
    populateFriendRows(mLlOutgoing, lists.outgoing, FriendRowAction.CANCEL_OUTGOING);
  }

  private void populateIncomingRows(@Nullable Friends.Friend[] friends)
  {
    mLlIncoming.removeAllViews();
    if (friends == null || friends.length == 0)
    {
      TextView empty = new TextView(requireContext());
      empty.setText(R.string.friends_none);
      mLlIncoming.addView(empty);
      return;
    }

    LayoutInflater inflater = LayoutInflater.from(requireContext());
    for (Friends.Friend friend : friends)
    {
      View row = inflater.inflate(R.layout.item_friend_row, mLlIncoming, false);
      TextView username = row.findViewById(R.id.tv_username);
      MaterialButton accept = row.findViewById(R.id.btn_action);
      MaterialButton decline = row.findViewById(R.id.btn_secondary_action);
      username.setText(friend.username);
      accept.setText(R.string.friends_accept);
      accept.setOnClickListener(v -> Friends.nativeAcceptRequest(friend.userId));
      decline.setVisibility(View.VISIBLE);
      decline.setText(R.string.friends_decline);
      decline.setOnClickListener(v -> Friends.nativeCancelRequest(friend.userId));
      mLlIncoming.addView(row);
    }
  }

  private void maybeHandlePendingAddFriend()
  {
    if (mPendingAddFriendUsername == null || mPendingAddFriendUsername.isEmpty())
      return;

    if (!Framework.nativeHasUsername())
    {
      mUsernameEdit.setText(mPendingAddFriendUsername);
      Toast.makeText(requireContext(), R.string.friends_signup_to_add, Toast.LENGTH_LONG).show();
      mPendingAddFriendUsername = null;
      return;
    }

    String username = mPendingAddFriendUsername;
    mPendingAddFriendUsername = null;
    mInputSearch.setText(username);
    Friends.nativeSearchByUsername(username, results -> {
      if (!isAdded())
        return;
      if (results == null || results.length == 0)
      {
        Toast.makeText(requireContext(), R.string.friends_no_search_results, Toast.LENGTH_SHORT).show();
        return;
      }
      Friends.nativeSendRequest(results[0].userId);
    });
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
    if (!isAdded())
      return;
    mBtnSignup.setEnabled(true);
    if (success)
    {
      Toast.makeText(requireContext(), R.string.friends_signup_success, Toast.LENGTH_SHORT).show();
      updateSignupVisibility();
      mUsernameLayout.setError(null);
      Friends.nativeRefresh();
      maybeHandlePendingAddFriend();
    }
    else
    {
      mUsernameLayout.setError(getString(R.string.friends_signup_error));
    }
  }

  @Override
  public void onUsernameChanged(boolean success)
  {
    if (!isAdded())
      return;
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
    if (!isAdded())
      return;
    if (success)
      Friends.nativeRefresh();
    else
      Toast.makeText(requireContext(), R.string.friends_action_failed, Toast.LENGTH_SHORT).show();
  }

  @Override
  public void onDeleteAccountResult(boolean success)
  {
    if (!isAdded())
      return;
    mBtnDeleteAccount.setEnabled(true);
    if (success)
    {
      Framework.nativeSetExploreSyncEnabled(false);
      Framework.nativeSetExploreFriendVisibilityEnabled(false);
      Toast.makeText(requireContext(), R.string.account_delete_success, Toast.LENGTH_SHORT).show();
      dismiss();
    }
    else
    {
      Toast.makeText(requireContext(), R.string.account_delete_failed, Toast.LENGTH_SHORT).show();
    }
  }

  @Override
  public void onExportAccountResult(boolean success, @Nullable String json)
  {
  }
}
