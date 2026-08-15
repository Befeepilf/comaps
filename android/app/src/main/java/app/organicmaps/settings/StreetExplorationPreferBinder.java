package app.organicmaps.settings;

import android.view.View;
import android.widget.SeekBar;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.StreetExplorationRoutingOptions;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.materialswitch.MaterialSwitch;

final class StreetExplorationPreferBinder
{
  private StreetExplorationPreferBinder() {}

  static void bind(@NonNull View root)
  {
    View avoidContainer = root.findViewById(R.id.street_exploration_avoid_container);
    if (avoidContainer != null)
      avoidContainer.setVisibility(View.GONE);

    View strengthContainer = root.findViewById(R.id.street_exploration_strength_container);
    MaterialSwitch preferUnexploredBtn = root.findViewById(R.id.prefer_unexplored_streets_btn);
    SeekBar strengthSeekBar = root.findViewById(R.id.street_exploration_strength_seekbar);

    preferUnexploredBtn.setOnCheckedChangeListener(null);
    strengthSeekBar.setOnSeekBarChangeListener(null);

    StreetExplorationRoutingOptions explorationOptions = StreetExplorationRoutingOptions.LoadFromSettings();
    preferUnexploredBtn.setChecked(explorationOptions.isPreferEnabled());
    strengthSeekBar.setProgress((int) explorationOptions.m_strength);
    strengthContainer.setVisibility(explorationOptions.isPreferEnabled() ? View.VISIBLE : View.GONE);

    preferUnexploredBtn.setOnCheckedChangeListener((buttonView, isChecked) -> {
      StreetExplorationRoutingOptions options = StreetExplorationRoutingOptions.LoadFromSettings();
      options.m_mode = isChecked ? StreetExplorationRoutingOptions.MODE_PREFER
                                 : StreetExplorationRoutingOptions.MODE_NEITHER;
      StreetExplorationRoutingOptions.SaveToSettings(options);
      strengthContainer.setVisibility(isChecked ? View.VISIBLE : View.GONE);
    });

    bindStrengthSeekBar(strengthSeekBar);
  }

  static void bindWithAvoid(@NonNull Fragment fragment, @NonNull View root)
  {
    View avoidContainer = root.findViewById(R.id.street_exploration_avoid_container);
    avoidContainer.setVisibility(View.VISIBLE);

    View strengthContainer = root.findViewById(R.id.street_exploration_strength_container);
    MaterialSwitch preferUnexploredBtn = root.findViewById(R.id.prefer_unexplored_streets_btn);
    MaterialSwitch avoidExploredBtn = root.findViewById(R.id.avoid_explored_streets_btn);
    SeekBar strengthSeekBar = root.findViewById(R.id.street_exploration_strength_seekbar);

    preferUnexploredBtn.setOnCheckedChangeListener(null);
    avoidExploredBtn.setOnCheckedChangeListener(null);
    strengthSeekBar.setOnSeekBarChangeListener(null);

    StreetExplorationRoutingOptions explorationOptions = StreetExplorationRoutingOptions.LoadFromSettings();
    preferUnexploredBtn.setChecked(explorationOptions.isPreferEnabled());
    avoidExploredBtn.setChecked(explorationOptions.isAvoidEnabled());
    strengthSeekBar.setProgress((int) explorationOptions.m_strength);
    strengthContainer.setVisibility(explorationOptions.isPreferEnabled() ? View.VISIBLE : View.GONE);

    final boolean[] updating = {false};

    preferUnexploredBtn.setOnCheckedChangeListener((buttonView, isChecked) -> {
      if (updating[0])
        return;
      StreetExplorationRoutingOptions options = StreetExplorationRoutingOptions.LoadFromSettings();
      options.m_mode = isChecked ? StreetExplorationRoutingOptions.MODE_PREFER
                                 : StreetExplorationRoutingOptions.MODE_NEITHER;
      StreetExplorationRoutingOptions.SaveToSettings(options);
      if (isChecked)
      {
        updating[0] = true;
        avoidExploredBtn.setChecked(false);
        updating[0] = false;
        strengthContainer.setVisibility(View.VISIBLE);
      }
      else
        strengthContainer.setVisibility(View.GONE);
    });

    avoidExploredBtn.setOnCheckedChangeListener((buttonView, isChecked) -> {
      if (updating[0])
        return;
      if (isChecked)
      {
        final boolean[] confirmed = {false};
        new MaterialAlertDialogBuilder(fragment.requireActivity())
            .setTitle(R.string.avoid_explored_streets)
            .setMessage(R.string.dialog_routing_avoid_explored_warning_message)
            .setPositiveButton(R.string.ok, (dialog, which) -> {
              confirmed[0] = true;
              StreetExplorationRoutingOptions options = StreetExplorationRoutingOptions.LoadFromSettings();
              options.m_mode = StreetExplorationRoutingOptions.MODE_AVOID;
              StreetExplorationRoutingOptions.SaveToSettings(options);
              updating[0] = true;
              preferUnexploredBtn.setChecked(false);
              updating[0] = false;
              strengthContainer.setVisibility(View.GONE);
            })
            .setNegativeButton(R.string.cancel, null)
            .setCancelable(true)
            .setOnDismissListener(dialog -> {
              if (confirmed[0])
                return;
              updating[0] = true;
              avoidExploredBtn.setChecked(false);
              updating[0] = false;
            })
            .show();
      }
      else
      {
        StreetExplorationRoutingOptions options = StreetExplorationRoutingOptions.LoadFromSettings();
        options.m_mode = StreetExplorationRoutingOptions.MODE_NEITHER;
        StreetExplorationRoutingOptions.SaveToSettings(options);
      }
    });

    bindStrengthSeekBar(strengthSeekBar);
  }

  private static void bindStrengthSeekBar(@NonNull SeekBar strengthSeekBar)
  {
    strengthSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener()
    {
      @Override
      public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
      {
        if (!fromUser)
          return;
        StreetExplorationRoutingOptions options = StreetExplorationRoutingOptions.LoadFromSettings();
        options.m_strength = progress;
        StreetExplorationRoutingOptions.SaveToSettings(options);
      }

      @Override
      public void onStartTrackingTouch(SeekBar seekBar) {}

      @Override
      public void onStopTrackingTouch(SeekBar seekBar) {}
    });
  }
}
