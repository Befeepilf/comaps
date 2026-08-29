package app.organicmaps.settings;
import androidx.annotation.Keep;

import android.os.Bundle;
import android.view.View;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.preference.TwoStatePreference;
import app.organicmaps.R;
import app.organicmaps.downloader.OnmapDownloader;
import app.organicmaps.sdk.ExplorerPro;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.util.Config;

@Keep
public class DataManagementSettingsFragment extends BaseXmlSettingsFragment
{
  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_data_management;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    initStoragePrefCallbacks();
    initBackupPrefCallback();
    initAutoDownloadPrefsCallbacks();
    initIncompleteSpaPref();
    initGpxToolsPref();
  }

  private void initGpxToolsPref()
  {
    boolean showImport = GpxSettingsVisibility.showImportRow(ExplorerPro.isGpxImportEnabled());
    boolean showExport = GpxSettingsVisibility.showExportRow(ExplorerPro.isGpxExportEnabled());
    boolean showBatch = GpxSettingsVisibility.showBatchImportRow(
        ExplorerPro.isGpxImportEnabled(), ExplorerPro.isAdvancedTrackManagementEnabled());
    boolean showInfo = GpxSettingsVisibility.showInfoPage(
        ExplorerPro.isGpxImportAvailable(), ExplorerPro.isGpxExportAvailable(),
        ExplorerPro.isAdvancedTrackManagementAvailable());
    boolean showScreen = GpxSettingsVisibility.showGpxScreen(showImport, showExport, showBatch, showInfo);
    String key = getString(R.string.pref_gpx_screen);
    Preference existing = getPreferenceScreen().findPreference(key);
    if (!showScreen)
    {
      if (existing != null)
        getPreferenceScreen().removePreference(existing);
      return;
    }
    if (existing != null)
      return;

    Preference pref = new Preference(requireContext());
    pref.setKey(key);
    pref.setTitle(R.string.pref_gpx_screen_title);
    pref.setSummary(R.string.pref_gpx_screen_summary);
    pref.setIcon(R.drawable.ic_file_gpx);
    pref.setPersistent(false);
    pref.setOnPreferenceClickListener(preference -> {
      getSettingsActivity().stackFragment(GpxSettingsFragment.class, getString(R.string.pref_gpx_screen_title), null);
      return true;
    });
    getPreferenceScreen().addPreference(pref);
  }

  private void initIncompleteSpaPref()
  {
    String key = getString(R.string.pref_incomplete_spa);
    Preference existing = getPreferenceScreen().findPreference(key);
    String[] ids = MapManager.nativeGetIncompleteSpaCountries();
    int count = ids == null ? 0 : ids.length;
    if (!IncompleteSpaSettingsVisibility.showRow(count))
    {
      if (existing != null)
        getPreferenceScreen().removePreference(existing);
      return;
    }
    Preference pref = existing;
    if (pref == null)
    {
      pref = new Preference(requireContext());
      pref.setKey(key);
      pref.setPersistent(false);
      pref.setIcon(R.drawable.ic_download);
      getPreferenceScreen().addPreference(pref);
    }
    pref.setTitle(R.string.pref_incomplete_spa_title);
    pref.setSummary(getString(R.string.pref_incomplete_spa_summary, count));
    pref.setOnPreferenceClickListener(preference -> {
      MapManager.nativeRetryIncompleteSpaDownloads();
      Toast.makeText(requireContext(), R.string.pref_incomplete_spa_retrying, Toast.LENGTH_SHORT).show();
      return true;
    });
  }

  private void initStoragePrefCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_storage));
    pref.setOnPreferenceClickListener(preference -> {
      if (app.organicmaps.sdk.downloader.MapManager.nativeIsDownloading())
      {
        new com.google.android.material.dialog.MaterialAlertDialogBuilder(requireActivity())
            .setTitle(R.string.downloading_is_active)
            .setMessage(R.string.cant_change_this_setting)
            .setPositiveButton(R.string.ok, null)
            .show();
      }
      else
      {
        getSettingsActivity().stackFragment(StoragePathFragment.class, getString(R.string.maps_storage), null);
      }
      return true;
    });
  }

  private void initBackupPrefCallback()
  {
    final Preference pref = getPreference(getString(R.string.pref_backup));
    pref.setOnPreferenceClickListener(preference -> {
      getSettingsActivity().stackFragment(BackupSettingsFragment.class, getString(R.string.pref_backup_title), null);
      return true;
    });
  }

  private void initAutoDownloadPrefsCallbacks()
  {
    final TwoStatePreference pref = getPreference(getString(R.string.pref_autodownload));
    pref.setChecked(Config.isAutodownloadEnabled());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      final boolean value = (boolean) newValue;
      Config.setAutodownloadEnabled(value);
      if (value)
        OnmapDownloader.setAutodownloadLocked(false);
      return true;
    });
  }
}
