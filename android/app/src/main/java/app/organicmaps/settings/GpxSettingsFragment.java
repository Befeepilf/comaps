package app.organicmaps.settings;
import androidx.annotation.Keep;

import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.widget.Toast;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.Preference;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.bookmarks.BookmarksSharingHelper;
import app.organicmaps.sdk.ExplorerPro;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.BookmarkSharingResult;
import app.organicmaps.sdk.bookmarks.data.KmlFileType;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.util.SharingUtils;
import app.organicmaps.util.Utils;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.io.File;
import java.util.Collections;
import java.util.List;

@Keep
public class GpxSettingsFragment extends BaseXmlSettingsFragment
    implements BookmarkManager.BookmarksSharingListener, BookmarkManager.BookmarksLoadingListener
{
  private static final String[] GPX_MIME_TYPES = {"application/gpx+xml", "application/gpx"};

  private ActivityResultLauncher<SharingUtils.SharingIntent> shareLauncher;

  private final ActivityResultLauncher<String[]> importLauncher =
      registerForActivityResult(new ActivityResultContracts.OpenDocument(), uri -> {
        if (uri != null)
          importGpxUris(Collections.singletonList(uri), false);
      });

  private final ActivityResultLauncher<String[]> batchImportLauncher =
      registerForActivityResult(new ActivityResultContracts.OpenMultipleDocuments(), uris -> {
        if (uris != null && !uris.isEmpty())
          importGpxUris(uris, true);
      });

  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_gpx;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    shareLauncher = SharingUtils.RegisterLauncher(this);
    applyGateVisibility();
    wireClickListeners();
  }

  @Override
  public void onStart()
  {
    super.onStart();
    BookmarkManager.INSTANCE.addLoadingListener(this);
    BookmarkManager.INSTANCE.addSharingListener(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    BookmarkManager.INSTANCE.removeLoadingListener(this);
    BookmarkManager.INSTANCE.removeSharingListener(this);
  }

  @Override
  public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
  {
    BookmarksSharingHelper.INSTANCE.onPreparedFileForSharing(requireActivity(), shareLauncher, result);
  }

  @Override
  public void onBookmarksFileImportFailed()
  {
    final View view = getView();
    if (view != null)
      Utils.showSnackbar(requireActivity(), view, R.string.load_kmz_failed);
  }

  private void applyGateVisibility()
  {
    removeIfHidden(getString(R.string.pref_gpx_import),
                   GpxSettingsVisibility.showImportRow(ExplorerPro.isGpxImportEnabled()));
    removeIfHidden(getString(R.string.pref_gpx_export),
                   GpxSettingsVisibility.showExportRow(ExplorerPro.isGpxExportEnabled()));
    removeIfHidden(getString(R.string.pref_gpx_batch_import),
                   GpxSettingsVisibility.showBatchImportRow(ExplorerPro.isGpxImportEnabled(),
                                                            ExplorerPro.isAdvancedTrackManagementEnabled()));
    removeIfHidden(getString(R.string.pref_gpx_info),
                   GpxSettingsVisibility.showInfoPage(ExplorerPro.isGpxImportAvailable(),
                                                      ExplorerPro.isGpxExportAvailable(),
                                                      ExplorerPro.isAdvancedTrackManagementAvailable()));
  }

  private void removeIfHidden(@NonNull String key, boolean show)
  {
    if (show)
      return;
    Preference pref = findPreference(key);
    if (pref != null)
      getPreferenceScreen().removePreference(pref);
  }

  private void wireClickListeners()
  {
    Preference importPref = findPreference(getString(R.string.pref_gpx_import));
    if (importPref != null)
    {
      importPref.setOnPreferenceClickListener(preference -> {
        if (!GpxSettingsVisibility.showImportRow(ExplorerPro.isGpxImportEnabled()))
          return true;
        launchDocumentPicker(importLauncher);
        return true;
      });
    }

    Preference batchPref = findPreference(getString(R.string.pref_gpx_batch_import));
    if (batchPref != null)
    {
      batchPref.setOnPreferenceClickListener(preference -> {
        if (!GpxSettingsVisibility.showBatchImportRow(ExplorerPro.isGpxImportEnabled(),
                                                      ExplorerPro.isAdvancedTrackManagementEnabled()))
          return true;
        launchDocumentPicker(batchImportLauncher);
        return true;
      });
    }

    Preference exportPref = findPreference(getString(R.string.pref_gpx_export));
    if (exportPref != null)
    {
      exportPref.setOnPreferenceClickListener(preference -> {
        if (!GpxSettingsVisibility.showExportRow(ExplorerPro.isGpxExportEnabled()))
          return true;
        showExportCategoryPicker();
        return true;
      });
    }

    Preference infoPref = findPreference(getString(R.string.pref_gpx_info));
    if (infoPref != null)
    {
      infoPref.setOnPreferenceClickListener(preference -> {
        if (!GpxSettingsVisibility.showInfoPage(ExplorerPro.isGpxImportAvailable(),
                                                ExplorerPro.isGpxExportAvailable(),
                                                ExplorerPro.isAdvancedTrackManagementAvailable()))
          return true;
        getSettingsActivity().stackFragment(ExplorerProInfoFragment.class,
                                            getString(R.string.explorer_pro_info_title), null);
        return true;
      });
    }
  }

  private void launchDocumentPicker(@NonNull ActivityResultLauncher<String[]> launcher)
  {
    try
    {
      launcher.launch(GPX_MIME_TYPES);
    }
    catch (ActivityNotFoundException e)
    {
      showNoFileManagerError();
    }
  }

  private void showNoFileManagerError()
  {
    new MaterialAlertDialogBuilder(requireActivity())
        .setMessage(R.string.error_no_file_manager_app)
        .setPositiveButton(android.R.string.ok, (dialog, which) -> dialog.dismiss())
        .show();
  }

  @SuppressWarnings("deprecation")
  private void importGpxUris(@NonNull List<Uri> uris, boolean batch)
  {
    final Context context = requireActivity();
    final ProgressDialog dialog = new ProgressDialog(context, R.style.MwmTheme_ProgressDialog);
    dialog.setMessage(getString(R.string.wait_several_minutes));
    dialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
    dialog.setIndeterminate(true);
    dialog.setCancelable(false);
    dialog.show();
    MwmApplication app = MwmApplication.from(context);
    final File tempDir = new File(StorageUtils.getTempPath(app));
    final ContentResolver resolver = context.getContentResolver();
    ThreadPool.getStorage().execute(() -> {
      int found;
      if (batch)
        found = BookmarkManager.INSTANCE.importBookmarksFiles(resolver, uris, tempDir);
      else
        found = BookmarkManager.INSTANCE.importBookmarksFile(resolver, uris.get(0), tempDir) ? 1 : 0;
      UiThread.run(() -> {
        if (dialog.isShowing())
          dialog.dismiss();
        if (!isAdded())
          return;
        String message =
            context.getResources().getQuantityString(R.plurals.bookmarks_detect_message, found, found);
        Toast.makeText(requireContext(), message, Toast.LENGTH_LONG).show();
      });
    });
  }

  private void showExportCategoryPicker()
  {
    List<BookmarkCategory> categories = BookmarkManager.INSTANCE.getCategories();
    if (categories.isEmpty())
    {
      new MaterialAlertDialogBuilder(requireActivity())
          .setTitle(R.string.bookmarks_error_title_share_empty)
          .setMessage(R.string.bookmarks_error_message_share_empty)
          .setPositiveButton(R.string.ok, null)
          .show();
      return;
    }

    final String[] names = new String[categories.size()];
    for (int i = 0; i < categories.size(); i++)
      names[i] = categories.get(i).getName();
    final List<BookmarkCategory> choices = categories;
    new MaterialAlertDialogBuilder(requireActivity())
        .setTitle(R.string.pref_gpx_export_pick_list)
        .setSingleChoiceItems(names, -1, (dialog, which) -> {
          dialog.dismiss();
          BookmarkCategory category = choices.get(which);
          BookmarksSharingHelper.INSTANCE.prepareBookmarkCategoryForSharing(requireActivity(), category.getId(),
                                                                            KmlFileType.Gpx);
        })
        .show();
  }
}
