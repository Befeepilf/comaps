package app.organicmaps.settings;

import android.view.View;
import android.widget.SeekBar;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.StreetExplorationRoutingOptions;
import com.google.android.material.materialswitch.MaterialSwitch;

final class StreetExplorationPreferBinder
{
  private StreetExplorationPreferBinder() {}

  static void bind(@NonNull View root)
  {
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
