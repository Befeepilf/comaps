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
    StreetExplorationRoutingOptions explorationOptions = StreetExplorationRoutingOptions.LoadFromSettings();
    View strengthContainer = root.findViewById(R.id.street_exploration_strength_container);
    MaterialSwitch preferUnexploredBtn = root.findViewById(R.id.prefer_unexplored_streets_btn);
    SeekBar strengthSeekBar = root.findViewById(R.id.street_exploration_strength_seekbar);

    preferUnexploredBtn.setChecked(explorationOptions.isPreferEnabled());
    strengthSeekBar.setProgress((int) explorationOptions.m_strength);
    strengthContainer.setVisibility(explorationOptions.isPreferEnabled() ? View.VISIBLE : View.GONE);

    preferUnexploredBtn.setOnCheckedChangeListener((buttonView, isChecked) -> {
      explorationOptions.m_mode = isChecked ? StreetExplorationRoutingOptions.MODE_PREFER
                                            : StreetExplorationRoutingOptions.MODE_NEITHER;
      StreetExplorationRoutingOptions.SaveToSettings(explorationOptions);
      strengthContainer.setVisibility(isChecked ? View.VISIBLE : View.GONE);
    });

    strengthSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener()
    {
      @Override
      public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
      {
        if (!fromUser)
          return;
        explorationOptions.m_strength = progress;
        StreetExplorationRoutingOptions.SaveToSettings(explorationOptions);
      }

      @Override
      public void onStartTrackingTouch(SeekBar seekBar) {}

      @Override
      public void onStopTrackingTouch(SeekBar seekBar) {}
    });
  }
}
