package app.organicmaps.maplayer.streetpixels;

import android.app.Dialog;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.LinearLayout;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.maplayer.streetpixels.CompetitionAreaChrome;
import app.organicmaps.sdk.maplayer.streetpixels.CompetitionRankingRow;
import app.organicmaps.sdk.maplayer.streetpixels.FocusedAreaProgress;
import app.organicmaps.sdk.maplayer.streetpixels.StreetPixelsManager;
import app.organicmaps.util.UiUtils;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.textview.MaterialTextView;
import java.util.Locale;
import java.util.Objects;

public class FocusedAreaDetailBottomSheet extends BottomSheetDialogFragment
{
  private static final String TAG = "FocusedAreaDetailBottomSheet";
  private static final String ARG_NAME = "name";
  private static final String ARG_FRACTION = "fraction";
  private static final String ARG_FRACTION_VALID = "fraction_valid";
  private static final String ARG_AREA_COMPLETED = "area_completed";
  private static final String ARG_PREVIOUSLY_COMPLETED = "previously_completed";
  private static final String ARG_EMPTY = "empty";
  private static final String ARG_OSM_ID = "osm_id";
  private static final String ARG_CITY_SUMMARY = "city_summary";

  public static void show(@NonNull FragmentManager fm, @NonNull String displayName, boolean fractionValid,
                          double fraction, boolean areaCompleted, boolean previouslyCompleted, long osmId,
                          boolean citySummary)
  {
    FocusedAreaDetailBottomSheet sheet = new FocusedAreaDetailBottomSheet();
    Bundle args = new Bundle();
    args.putString(ARG_NAME, displayName);
    args.putBoolean(ARG_FRACTION_VALID, fractionValid);
    args.putDouble(ARG_FRACTION, fraction);
    args.putBoolean(ARG_AREA_COMPLETED, areaCompleted);
    args.putBoolean(ARG_PREVIOUSLY_COMPLETED, previouslyCompleted);
    args.putBoolean(ARG_EMPTY, false);
    args.putLong(ARG_OSM_ID, osmId);
    args.putBoolean(ARG_CITY_SUMMARY, citySummary);
    sheet.setArguments(args);
    dismissIfShowing(fm);
    sheet.show(fm, TAG);
  }

  public static void showEmpty(@NonNull FragmentManager fm)
  {
    FocusedAreaDetailBottomSheet sheet = new FocusedAreaDetailBottomSheet();
    Bundle args = new Bundle();
    args.putBoolean(ARG_EMPTY, true);
    sheet.setArguments(args);
    dismissIfShowing(fm);
    sheet.show(fm, TAG);
  }

  public static void dismissIfShowing(@NonNull FragmentManager fm)
  {
    Fragment existing = fm.findFragmentByTag(TAG);
    if (existing instanceof FocusedAreaDetailBottomSheet)
      ((FocusedAreaDetailBottomSheet) existing).dismissAllowingStateLoss();
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
  {
    return new BottomSheetDialog(requireContext(), R.style.MwmTheme_BottomSheetDialog) {
      @Override
      public void onAttachedToWindow()
      {
        super.onAttachedToWindow();
        Window window = Objects.requireNonNull(getWindow());
        WindowInsetsControllerCompat insetsController = WindowCompat.getInsetsController(window, window.getDecorView());
        insetsController.setAppearanceLightNavigationBars(false);
      }
    };
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.focused_area_detail_bottom_sheet, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    Bundle args = requireArguments();
    MaterialTextView nameView = view.findViewById(R.id.focused_area_detail_name);
    MaterialTextView percentView = view.findViewById(R.id.focused_area_detail_percent);
    MaterialTextView bodyView = view.findViewById(R.id.focused_area_detail_body);
    View competitionBlock = view.findViewById(R.id.competition_block);

    if (args.getBoolean(ARG_EMPTY, false))
    {
      Log.i("StreetPixels", "sheet bind empty");
      nameView.setText(R.string.street_pixels_no_exploration_area_title);
      percentView.setText("");
      bodyView.setText(R.string.street_pixels_no_exploration_area_message);
      UiUtils.show(bodyView);
      if (competitionBlock != null)
        UiUtils.hide(competitionBlock);
      return;
    }

    UiUtils.hide(bodyView);
    nameView.setText(args.getString(ARG_NAME, ""));
    boolean areaCompleted = args.getBoolean(ARG_AREA_COMPLETED, false);
    boolean fractionValid = args.getBoolean(ARG_FRACTION_VALID, false);
    double fraction = args.getDouble(ARG_FRACTION, 0.0);
    Log.i("StreetPixels",
          "sheet bind fractionValid=" + fractionValid + " fraction=" + fraction + " areaCompleted=" + areaCompleted
              + " omitPercent=" + !fractionValid);
    if (fractionValid)
    {
      if (areaCompleted)
        percentView.setText(R.string.street_pixels_area_completed);
      else
        percentView.setText(FocusedAreaProgress.formatPercent(fraction));
    }
    else
      percentView.setText("");
    if (args.getBoolean(ARG_PREVIOUSLY_COMPLETED, false) && !areaCompleted)
    {
      bodyView.setText(R.string.street_pixels_area_previously_completed);
      UiUtils.show(bodyView);
    }
    bindCompetitionBlock(view, args);
  }

  private void bindCompetitionBlock(@NonNull View view, @NonNull Bundle args)
  {
    View competitionBlock = view.findViewById(R.id.competition_block);
    if (competitionBlock == null)
      return;
    long osmId = args.getLong(ARG_OSM_ID, 0L);
    boolean citySummary = args.getBoolean(ARG_CITY_SUMMARY, false);
    boolean showCompetition = Framework.nativeHasExploreConsent() && Framework.nativeGetCompetitionMapMode() == 1
                              && osmId != 0;
    StreetPixelsManager manager = MwmApplication.from(requireContext()).getStreetPixelsManager();
    if (!showCompetition)
    {
      UiUtils.hide(competitionBlock);
      if (Framework.nativeHasExploreConsent() && osmId != 0)
      {
        manager.requestCompetitionAreaSnapshot(osmId, chrome -> {
          if (!isAdded())
            return;
          maybeShowOvertakingHint(manager);
        });
      }
      return;
    }

    UiUtils.show(competitionBlock);
    applyCompetitionChrome(view, manager.getCompetitionAreaChrome(osmId), citySummary);
    manager.requestCompetitionAreaSnapshot(osmId, chrome -> {
      if (!isAdded())
        return;
      View bound = getView();
      if (bound == null)
        return;
      applyCompetitionChrome(bound, chrome, citySummary);
      maybeShowOvertakingHint(manager);
    });
  }

  private void applyCompetitionChrome(@NonNull View view, @NonNull CompetitionAreaChrome chrome, boolean citySummary)
  {
    MaterialTextView statusView = view.findViewById(R.id.competition_status);
    if (chrome.offline)
    {
      statusView.setText(R.string.competition_status_offline);
      UiUtils.show(statusView);
    }
    else if (chrome.stale)
    {
      statusView.setText(R.string.competition_status_stale);
      UiUtils.show(statusView);
    }
    else
      UiUtils.hide(statusView);

    MaterialTextView stateLabel = view.findViewById(R.id.competition_state_label);
    if (chrome.unclaimed)
    {
      stateLabel.setText(R.string.competition_unclaimed);
      UiUtils.show(stateLabel);
    }
    else if (chrome.contested)
    {
      stateLabel.setText(R.string.competition_contested);
      UiUtils.show(stateLabel);
    }
    else
      UiUtils.hide(stateLabel);

    MaterialTextView bossView = view.findViewById(R.id.competition_boss_line);
    if (chrome.bossLine.isEmpty())
      UiUtils.hide(bossView);
    else
    {
      bossView.setText(chrome.bossLine);
      UiUtils.show(bossView);
    }

    MaterialTextView gapView = view.findViewById(R.id.competition_gap_line);
    if (chrome.gapLine.isEmpty())
      UiUtils.hide(gapView);
    else
    {
      gapView.setText(chrome.gapLine);
      UiUtils.show(gapView);
    }

    MaterialTextView ownershipView = view.findViewById(R.id.competition_ownership_score);
    ownershipView.setText(getString(R.string.competition_ownership_score, chrome.localOwnershipScore));
    UiUtils.show(ownershipView);

    MaterialTextView personalView = view.findViewById(R.id.competition_personal_completion);
    personalView.setText(getString(R.string.competition_personal_completion,
                                   FocusedAreaProgress.formatPercent(chrome.personalCompletionFraction)));
    UiUtils.show(personalView);

    LinearLayout rankingRows = view.findViewById(R.id.competition_ranking_rows);
    rankingRows.removeAllViews();
    int count = Math.min(chrome.rankingRows.length, 4);
    if (count == 0)
      UiUtils.hide(rankingRows);
    else
    {
      UiUtils.show(rankingRows);
      for (int i = 0; i < count; ++i)
      {
        CompetitionRankingRow row = chrome.rankingRows[i];
        MaterialTextView line = new MaterialTextView(requireContext());
        line.setTextAppearance(R.style.MwmTextAppearance_Body3);
        line.setText(row.rank + "  " + row.displayName + "  "
                     + String.format(Locale.US, "%.1f", row.decayedScore));
        rankingRows.addView(line, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                                ViewGroup.LayoutParams.WRAP_CONTENT));
      }
    }

    MaterialTextView weeklyTitle = view.findViewById(R.id.competition_weekly_title);
    MaterialTextView weeklyBody = view.findViewById(R.id.competition_weekly_body);
    if (citySummary)
    {
      weeklyTitle.setText(R.string.competition_weekly_title);
      weeklyBody.setText(R.string.competition_weekly_empty);
      UiUtils.show(weeklyTitle);
      UiUtils.show(weeklyBody);
    }
    else
    {
      UiUtils.hide(weeklyTitle);
      UiUtils.hide(weeklyBody);
    }
  }

  private void maybeShowOvertakingHint(@NonNull StreetPixelsManager manager)
  {
    String text = manager.tryConsumeOvertakingHint(Framework.nativeIsRoutingFollowing());
    if (text == null || text.isEmpty())
      return;
    Toast.makeText(requireContext(), text, Toast.LENGTH_LONG).show();
  }
}
