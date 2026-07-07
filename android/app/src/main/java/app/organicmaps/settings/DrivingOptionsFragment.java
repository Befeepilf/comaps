package app.organicmaps.settings;

import android.app.Activity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.routing.StreetExplorationRoutingOptions;
import app.organicmaps.sdk.settings.RoadType;
import com.google.android.material.materialswitch.MaterialSwitch;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class DrivingOptionsFragment extends Fragment
{
  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_driving_options, container, false);
    initViews(root);
    return root;
  }

  private void initViews(@NonNull View root)
  {
    MaterialSwitch tollsBtn = root.findViewById(R.id.avoid_tolls_vehicle_btn);
    tollsBtn.setChecked(RoutingOptions.hasOption(RoadType.Toll, Router.Vehicle));
    CompoundButton.OnCheckedChangeListener tollBtnListener =
        new ToggleRoutingOptionListener(RoadType.Toll, root, Router.Vehicle);
    tollsBtn.setOnCheckedChangeListener(tollBtnListener);

    MaterialSwitch motorwaysBtn = root.findViewById(R.id.avoid_motorways_vehicle_btn);
    motorwaysBtn.setChecked(RoutingOptions.hasOption(RoadType.Motorway, Router.Vehicle));
    CompoundButton.OnCheckedChangeListener motorwayBtnListener =
        new ToggleRoutingOptionListener(RoadType.Motorway, root, Router.Vehicle);
    motorwaysBtn.setOnCheckedChangeListener(motorwayBtnListener);

    MaterialSwitch ferriesBtn = root.findViewById(R.id.avoid_ferries_vehicle_btn);
    ferriesBtn.setChecked(RoutingOptions.hasOption(RoadType.Ferry, Router.Vehicle));
    CompoundButton.OnCheckedChangeListener ferryBtnListener =
        new ToggleRoutingOptionListener(RoadType.Ferry, root, Router.Vehicle);
    ferriesBtn.setOnCheckedChangeListener(ferryBtnListener);

    MaterialSwitch dirtyRoadsBtn = root.findViewById(R.id.avoid_dirty_roads_vehicle_btn);
    dirtyRoadsBtn.setChecked(RoutingOptions.hasOption(RoadType.Dirty, Router.Vehicle));
    CompoundButton.OnCheckedChangeListener dirtyBtnListener =
        new ToggleRoutingOptionListener(RoadType.Dirty, root, Router.Vehicle);
    dirtyRoadsBtn.setOnCheckedChangeListener(dirtyBtnListener);

    MaterialSwitch pavedBtn = root.findViewById(R.id.avoid_paved_roads_vehicle_btn);
    pavedBtn.setChecked(RoutingOptions.hasOption(RoadType.Paved, Router.Vehicle));
    CompoundButton.OnCheckedChangeListener pavedBtnListener =
        new ToggleRoutingOptionListener(RoadType.Paved, root, Router.Vehicle);
    pavedBtn.setOnCheckedChangeListener(pavedBtnListener);

    StreetExplorationRoutingOptions explorationOptions = StreetExplorationRoutingOptions.LoadFromSettings();

    View strengthContainer = root.findViewById(R.id.street_exploration_strength_container);
    MaterialSwitch preferUnexploredBtn = root.findViewById(R.id.prefer_unexplored_streets_btn);
    SeekBar strengthSeekBar = root.findViewById(R.id.street_exploration_strength_seekbar);

    preferUnexploredBtn.setChecked(explorationOptions.m_enabled);
    strengthSeekBar.setProgress((int) explorationOptions.m_strength);
    strengthContainer.setVisibility(explorationOptions.m_enabled ? View.VISIBLE : View.GONE);

    preferUnexploredBtn.setOnCheckedChangeListener((buttonView, isChecked) -> {
      explorationOptions.m_enabled = isChecked;
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

  private record
      ToggleRoutingOptionListener(@NonNull RoadType mRoadType, @NonNull View mRoot, @NonNull Router mRouterType)
      implements CompoundButton.OnCheckedChangeListener {
    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
    {
      if (isChecked)
      {
        if (mRoadType == RoadType.Dirty)
        {
          MaterialSwitch btn = mRoot.findViewById(R.id.avoid_paved_roads_vehicle_btn);
          btn.setChecked(false);
        }
        else if (mRoadType == RoadType.Paved)
        {
          MaterialSwitch btn = mRoot.findViewById(R.id.avoid_dirty_roads_vehicle_btn);
          btn.setChecked(false);
        }
        RoutingOptions.addOption(mRoadType, mRouterType);
      }
      else
        RoutingOptions.removeOption(mRoadType, mRouterType);
    }
  }
}
