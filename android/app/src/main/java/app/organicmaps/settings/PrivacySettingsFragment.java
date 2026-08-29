package app.organicmaps.settings;
import android.annotation.SuppressLint;
import android.os.Bundle;
import android.view.View;
import android.widget.Toast;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.preference.TwoStatePreference;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.search.SearchRecents;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

@Keep
public class PrivacySettingsFragment extends BaseXmlSettingsFragment
{
  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_privacy;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    initPlayServicesPrefsCallbacks();
    initSearchPrivacyPrefsCallbacks();
    hideFriendRows();
    initCompetitionEnabledPref();
    initPublicNicknamePref();
    initDeleteCompetitionPref();
    initPrivacyInformationPref();
    initCompetitionRulesPref();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    refreshCompetitionEnabledPref();
    refreshPublicNicknamePref();
  }

  private void hideFriendRows()
  {
    if (FriendSettingsVisibility.showFriendRows(FriendSettingsVisibility.friendsCapabilityEnabled()))
      return;
    Preference friend = findPreference(getString(R.string.pref_explore_friend_visibility));
    if (friend != null)
      getPreferenceScreen().removePreference(friend);
  }

  private void initCompetitionEnabledPref()
  {
    final TwoStatePreference pref = getPreference(getString(R.string.pref_competition_enabled));
    refreshCompetitionEnabledPref();
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      boolean enabled = (Boolean) newValue;
      if (enabled && !Framework.nativeHasExploreConsent())
      {
        ExploreConsentDialogFragment.maybeShow(getParentFragmentManager(), new ExploreConsentDialogFragment.Listener() {
          @Override
          public void onExploreConsentGranted()
          {
            Framework.nativeSetExploreSyncEnabled(true);
            refreshCompetitionEnabledPref();
          }

          @Override
          public void onExploreConsentDeclined()
          {
            refreshCompetitionEnabledPref();
          }
        });
        return false;
      }
      Framework.nativeSetExploreSyncEnabled(enabled);
      return true;
    });
  }

  private void refreshCompetitionEnabledPref()
  {
    final TwoStatePreference pref = getPreference(getString(R.string.pref_competition_enabled));
    pref.setChecked(Framework.nativeGetExploreSyncEnabled());
  }

  private void initPublicNicknamePref()
  {
    final Preference pref = getPreference(getString(R.string.pref_public_nickname));
    refreshPublicNicknamePref();
    pref.setOnPreferenceClickListener(preference -> {
      MyAccountDialogFragment.show(getParentFragmentManager());
      return true;
    });
  }

  private void refreshPublicNicknamePref()
  {
    final Preference pref = getPreference(getString(R.string.pref_public_nickname));
    if (Framework.nativeHasUsername())
      pref.setSummary(Framework.nativeGetUsername());
    else
      pref.setSummary(R.string.pref_explore_username_summary);
  }

  private void initDeleteCompetitionPref()
  {
    final Preference pref = getPreference(getString(R.string.pref_delete_competition_profile));
    pref.setOnPreferenceClickListener(preference -> {
      new MaterialAlertDialogBuilder(requireContext())
          .setTitle(R.string.competition_delete_confirm_title)
          .setMessage(R.string.competition_delete_confirm_message)
          .setPositiveButton(R.string.competition_delete, (d, w) -> deleteCompetitionProfile())
          .setNegativeButton(R.string.cancel, null)
          .show();
      return true;
    });
  }

  private void deleteCompetitionProfile()
  {
    ThreadPool.getWorker().execute(() -> {
      int result = Framework.nativeDeleteCompetitionProfile();
      UiThread.run(() -> {
        if (!isAdded())
          return;
        if (result != 0)
        {
          Toast.makeText(requireContext(), R.string.competition_delete_unavailable, Toast.LENGTH_LONG).show();
          return;
        }
        refreshCompetitionEnabledPref();
        refreshPublicNicknamePref();
      });
    });
  }

  private void initPrivacyInformationPref()
  {
    final Preference pref = getPreference(getString(R.string.pref_privacy_information));
    pref.setOnPreferenceClickListener(preference -> {
      String message = getString(R.string.location_privacy_info) + "\n\n" + getString(R.string.explore_consent_message);
      new MaterialAlertDialogBuilder(requireContext())
          .setTitle(R.string.location_privacy)
          .setMessage(message)
          .setPositiveButton(R.string.close, null)
          .show();
      return true;
    });
  }

  private void initCompetitionRulesPref()
  {
    final Preference pref = getPreference(getString(R.string.pref_competition_rules));
    pref.setOnPreferenceClickListener(preference -> {
      String message = getString(R.string.explore_consent_message) + "\n\n"
                     + getString(R.string.competition_leave_confirm_message) + "\n\n"
                     + getString(R.string.competition_delete_confirm_message);
      new MaterialAlertDialogBuilder(requireContext())
          .setTitle(R.string.pref_competition_rules_title)
          .setMessage(message)
          .setPositiveButton(R.string.close, null)
          .show();
      return true;
    });
  }

  private void initPlayServicesPrefsCallbacks()
  {
    final Preference pref = findPreference(getString(R.string.pref_play_services));
    if (pref == null)
      return;

    if (!MwmApplication.from(requireContext())
             .getLocationProviderFactory()
             .isGoogleLocationAvailable(requireActivity().getApplicationContext()))
    {
      pref.setVisible(false);
    }
    else
    {
      ((TwoStatePreference) pref).setChecked(Config.useGoogleServices());
      pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
        @SuppressLint("MissingPermission")
        @Override
        public boolean onPreferenceChange(@NonNull Preference preference, Object newValue)
        {
          final LocationHelper locationHelper = MwmApplication.from(requireContext()).getLocationHelper();
          boolean oldVal = Config.useGoogleServices();
          boolean newVal = (Boolean) newValue;
          if (oldVal != newVal)
          {
            Config.setUseGoogleService(newVal);
            if (locationHelper.isActive())
            {
              locationHelper.stop();
              locationHelper.start();
            }
          }
          return true;
        }
      });
    }
  }

  private void initSearchPrivacyPrefsCallbacks()
  {
    final Preference pref = findPreference(getString(R.string.pref_search_history));
    if (pref == null)
      return;

    final boolean isHistoryEnabled = Config.isSearchHistoryEnabled();
    ((TwoStatePreference) pref).setChecked(isHistoryEnabled);
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      boolean newVal = (Boolean) newValue;
      if (newVal != Config.isSearchHistoryEnabled())
      {
        Config.setSearchHistoryEnabled(newVal);
        if (newVal)
          SearchRecents.refresh();
        else
          SearchRecents.clear();
      }
      return true;
    });
  }
}
