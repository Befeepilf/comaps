package app.organicmaps.settings;
import androidx.annotation.Keep;

import android.os.Bundle;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.CheckBoxPreference;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.SeekBarPreference;
import androidx.preference.TwoStatePreference;
import app.organicmaps.R;
import app.organicmaps.editor.MapLanguagesFragment;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.editor.data.Language;
import app.organicmaps.sdk.settings.MapLanguageCode;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.PowerManagment;
import java.util.Locale;
import java.util.function.IntConsumer;

@Keep
public class MapAppearanceSettingsFragment extends BaseXmlSettingsFragment implements MapLanguagesFragment.Listener
{
  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_map_appearance;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    init3dModePrefsCallbacks();
    initLargeFontSizePrefsCallbacks();
    initTransliterationPrefsCallbacks();
    initAlternativeMapLanguageHandlingCallbacks();
    initExplorationAreasPrefs();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    updateMapLanguageCodeSummary();
  }

  @Override
  public boolean onPreferenceTreeClick(Preference preference)
  {
    final String key = preference.getKey();
    if (key == null)
      return super.onPreferenceTreeClick(preference);

    if (key.equals(getString(R.string.pref_map_locale)))
    {
      MapLanguagesFragment langFragment = (MapLanguagesFragment) getSettingsActivity().stackFragment(
          MapLanguagesFragment.class, getString(R.string.change_map_locale), null);
      langFragment.setListener(this);
    }
    return super.onPreferenceTreeClick(preference);
  }

  @Override
  public void onMapLanguageSelected(Language language)
  {
    MapLanguageCode.setMapLanguageCode(language.code);
    getSettingsActivity().onBackPressed();
  }

  private void updateMapLanguageCodeSummary()
  {
    final Preference pref = getPreference(getString(R.string.pref_map_locale));
    String mapLanguageCode = MapLanguageCode.getMapLanguageCode();
    if (mapLanguageCode.equals(Language.AUTO_LANG_CODE))
      pref.setSummary(R.string.auto);
    else if (mapLanguageCode.equals(Language.DEFAULT_LANG_CODE))
      pref.setSummary(R.string.pref_maplanguage_local);
    else
      pref.setSummary(new Locale(mapLanguageCode).getDisplayLanguage());
  }

  private void initLargeFontSizePrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_large_fonts_size));
    ((TwoStatePreference) pref).setChecked(Config.isLargeFontsSize());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      final boolean oldVal = Config.isLargeFontsSize();
      final boolean newVal = (Boolean) newValue;
      if (oldVal != newVal)
        Config.setLargeFontsSize(newVal);
      return true;
    });
  }

  private void initAlternativeMapLanguageHandlingCallbacks()
  {
    final ListPreference pref = getPreference(getString(R.string.pref_alt_map_lang_handling_key));
    pref.setValue(String.valueOf(Config.getAlternativeMapLanguageHandling()));
    pref.setSummary(pref.getEntry());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      final int alternativeMapLanguageHandling = Integer.parseInt((String) newValue);
      Config.setAlternativeMapLanguageHandling(alternativeMapLanguageHandling);
      preference.setSummary(pref.getEntries()[alternativeMapLanguageHandling]);
      return true;
    });
  }

  private void initTransliterationPrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_transliteration));
    ((TwoStatePreference) pref).setChecked(Config.isTransliteration());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      final boolean oldVal = Config.isTransliteration();
      final boolean newVal = (Boolean) newValue;
      if (oldVal != newVal)
        Config.setTransliteration(newVal);
      return true;
    });
  }

  private void init3dModePrefsCallbacks()
  {
    final TwoStatePreference pref = getPreference(getString(R.string.pref_3d_buildings));
    final Framework.Params3dMode _3d = new Framework.Params3dMode();
    Framework.nativeGet3dMode(_3d);

    // Check power management: high-power mode disables 3D buildings
    @PowerManagment.SchemeType int powerScheme = PowerManagment.getScheme();
    if (powerScheme == PowerManagment.HIGH)
    {
      pref.setShouldDisableView(true);
      pref.setEnabled(false);
      pref.setSummary(getString(R.string.pref_map_3d_buildings_disabled_summary));
      pref.setChecked(false);
    }
    else
    {
      pref.setShouldDisableView(false);
      pref.setEnabled(true);
      pref.setSummary("");
      pref.setChecked(_3d.buildings);
    }

    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      Framework.Params3dMode current = new Framework.Params3dMode();
      Framework.nativeGet3dMode(current);
      Framework.nativeSet3dMode(current.enabled, (Boolean) newValue);
      return true;
    });
  }

  private void initExplorationAreasPrefs()
  {
    CheckBoxPreference showName = getPreference(getString(R.string.pref_exploration_areas_show_name));
    showName.setChecked(Config.showExplorationAreaName());
    showName.setOnPreferenceChangeListener((preference, newValue) -> {
      Config.setShowExplorationAreaName((Boolean) newValue);
      Framework.nativeApplyExplorationAreaOverlayPrefs();
      return true;
    });

    CheckBoxPreference showPct = getPreference(getString(R.string.pref_exploration_areas_show_pct));
    showPct.setChecked(Config.showExplorationAreaPercent());
    showPct.setOnPreferenceChangeListener((preference, newValue) -> {
      Config.setShowExplorationAreaPercent((Boolean) newValue);
      Framework.nativeApplyExplorationAreaOverlayPrefs();
      return true;
    });

    SeekBarPreference fontSize = getPreference(getString(R.string.pref_exploration_areas_font_size));
    fontSize.setMin(Config.EXPLORATION_AREAS_FONT_SIZE_MIN);
    fontSize.setMax(Config.EXPLORATION_AREAS_FONT_SIZE_MAX);
    fontSize.setValue(Config.explorationAreaFontSize());
    fontSize.setOnPreferenceChangeListener((preference, newValue) -> {
      Config.setExplorationAreaFontSize((Integer) newValue);
      Framework.nativeApplyExplorationAreaOverlayPrefs();
      return true;
    });

    SeekBarPreference fillOpacity = getPreference(getString(R.string.pref_exploration_areas_fill_opacity));
    fillOpacity.setMin(0);
    fillOpacity.setMax(100);
    fillOpacity.setValue(Config.explorationAreaFillOpacity());
    fillOpacity.setOnPreferenceChangeListener((preference, newValue) -> {
      Config.setExplorationAreaFillOpacity((Integer) newValue);
      Framework.nativeApplyExplorationAreaOverlayPrefs();
      return true;
    });

    bindZoomRange(getString(R.string.pref_exploration_areas_label_min_zoom),
                  getString(R.string.pref_exploration_areas_label_max_zoom), Config.explorationAreaLabelMinZoom(),
                  Config.explorationAreaLabelMaxZoom(), Config::setExplorationAreaLabelMinZoom,
                  Config::setExplorationAreaLabelMaxZoom);
    bindZoomRange(getString(R.string.pref_exploration_areas_fill_min_zoom),
                  getString(R.string.pref_exploration_areas_fill_max_zoom), Config.explorationAreaFillMinZoom(),
                  Config.explorationAreaFillMaxZoom(), Config::setExplorationAreaFillMinZoom,
                  Config::setExplorationAreaFillMaxZoom);
  }

  private void bindZoomRange(String minKey, String maxKey, int minVal, int maxVal, IntConsumer setMin,
                             IntConsumer setMax)
  {
    SeekBarPreference minPref = getPreference(minKey);
    SeekBarPreference maxPref = getPreference(maxKey);
    minPref.setMin(Config.EXPLORATION_AREAS_ZOOM_MIN);
    minPref.setMax(Config.EXPLORATION_AREAS_ZOOM_MAX);
    maxPref.setMin(Config.EXPLORATION_AREAS_ZOOM_MIN);
    maxPref.setMax(Config.EXPLORATION_AREAS_ZOOM_MAX);
    minPref.setValue(minVal);
    maxPref.setValue(maxVal);
    minPref.setOnPreferenceChangeListener((preference, newValue) -> {
      int min = (Integer) newValue;
      int max = maxPref.getValue();
      if (min > max)
      {
        max = min;
        maxPref.setValue(max);
        setMax.accept(max);
      }
      setMin.accept(min);
      Framework.nativeApplyExplorationAreaOverlayPrefs();
      return true;
    });
    maxPref.setOnPreferenceChangeListener((preference, newValue) -> {
      int max = (Integer) newValue;
      int min = minPref.getValue();
      if (max < min)
      {
        min = max;
        minPref.setValue(min);
        setMin.accept(min);
      }
      setMax.accept(max);
      Framework.nativeApplyExplorationAreaOverlayPrefs();
      return true;
    });
  }
}
