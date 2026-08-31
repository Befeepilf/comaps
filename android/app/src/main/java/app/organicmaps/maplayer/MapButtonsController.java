package app.organicmaps.maplayer;

import static app.organicmaps.leftbutton.LeftButtonsHolder.DISABLE_BUTTON_CODE;

import android.animation.ArgbEvaluator;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.text.TextUtils;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.OptIn;
import androidx.core.content.ContextCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.leftbutton.LeftButton;
import app.organicmaps.leftbutton.LeftToggleButton;
import app.organicmaps.MwmApplication;
import app.organicmaps.location.GpsWaitingState;
import app.organicmaps.maplayer.streetpixels.FocusedAreaDetailBottomSheet;
import app.organicmaps.settings.ExploreConsentDialogFragment;
import app.organicmaps.settings.MyAccountDialogFragment;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.downloader.UpdateInfo;
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.location.RecordingSession;
import app.organicmaps.location.RecordingSessionUiModel;
import app.organicmaps.sdk.maplayer.isolines.IsolinesManager;
import app.organicmaps.sdk.maplayer.streetpixels.AreaMilestonePresentation;
import app.organicmaps.sdk.maplayer.streetpixels.CompletionCardModel;
import app.organicmaps.sdk.maplayer.streetpixels.CompletionCardSharePayload;
import app.organicmaps.sdk.maplayer.streetpixels.FirstGoalProgress;
import app.organicmaps.sdk.maplayer.streetpixels.FocusedAreaProgress;
import app.organicmaps.sdk.maplayer.streetpixels.StreetPixelsManager;
import app.organicmaps.sdk.maplayer.streetpixels.StreetPixelsState;
import app.organicmaps.sdk.maplayer.subway.SubwayManager;
import app.organicmaps.sdk.maplayer.traffic.TrafficManager;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.WindowInsetUtils;
import app.organicmaps.widget.menu.MyPositionButton;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import com.google.android.material.badge.BadgeDrawable;
import com.google.android.material.badge.BadgeUtils;
import com.google.android.material.badge.ExperimentalBadgeUtils;
import com.google.android.material.button.MaterialButtonToggleGroup;
import com.google.android.material.floatingactionbutton.ExtendedFloatingActionButton;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import java.util.HashMap;
import java.util.Map;

public class MapButtonsController extends Fragment implements LocationListener
{
  Map<MapButtons, View> mButtonsMap;
  private View mFrame;
  private View mInnerLeftButtonsFrame;
  private View mInnerRightButtonsFrame;
  @Nullable
  private View mBottomButtonsFrame;
  @Nullable
  private LayersButton mToggleMapLayerButton;
  @Nullable
  FloatingActionButton mTrackRecordingStatusButton;
  @Nullable
  private ExtendedFloatingActionButton mExplorationBadge;
  @Nullable
  private ExtendedFloatingActionButton mGpsWaitingBadge;
  private boolean mGpsTimedOut;
  @Nullable
  private ExtendedFloatingActionButton mFirstGoalBadge;
  @Nullable
  private MaterialButtonToggleGroup mCompetitionModeToggle;
  @Nullable
  private ExtendedFloatingActionButton mCompetitionHintBadge;
  @Nullable
  private View mCompletionCard;
  @Nullable
  private TextView mCompletionCardTitle;
  @Nullable
  private TextView mCompletionCardBody;
  @Nullable
  private CompletionCardOutlineView mCompletionCardOutline;
  @Nullable
  private TextView mCompletionCardNickname;
  @Nullable
  private TextView mCompletionCardDate;
  @Nullable
  private TextView mCompletionCardCompetition;
  @Nullable
  private TextView mCompletionCardBranding;
  @Nullable
  private ObjectAnimator mTrackRecordingBlinkAnimator;

  @Nullable
  private MyPositionButton mNavMyPosition;
  private SearchWheel mSearchWheel;
  private BadgeDrawable mBadgeDrawable;
  private float mContentHeight;
  private float mContentWidth;

  private MapButtonClickListener mMapButtonClickListener;
  private PlacePageViewModel mPlacePageViewModel;
  private MapButtonsViewModel mMapButtonsViewModel;

  private final Observer<Integer> mPlacePageDistanceToTopObserver = this::move;
  private final Observer<Boolean> mButtonHiddenObserver = this::setButtonsHidden;
  private final Observer<Integer> mMyPositionModeObserver = this::updateNavMyPositionButton;
  private final Observer<SearchWheel.SearchOption> mSearchOptionObserver = this::onSearchOptionChange;
  private final Observer<Integer> mRecordingSessionObserver = this::onRecordingSessionStateChanged;
  private final Observer<StreetPixelsState> mStreetPixelsStateObserver = this::updateExplorationBadge;
  private final StreetPixelsManager.FocusedAreaProgressCallback mFocusedAreaProgressCallback =
      this::bindExplorationBadgeFromProgress;
  private final StreetPixelsManager.FirstGoalProgressCallback mFirstGoalProgressCallback =
      this::applyFirstGoalBadge;
  private final StreetPixelsManager.AreaMilestonePresentationCallback mAreaMilestonePresentationCallback =
      this::applyAreaMilestonePresentation;
  private final StreetPixelsManager.CompetitionHintCallback mCompetitionHintCallback = this::onCompetitionHintReady;
  private final Handler mCompetitionHintHandler = new Handler(Looper.getMainLooper());
  private final Runnable mHideCompetitionHint = this::hideCompetitionHintBadge;
  private boolean mUpdatingCompetitionToggle;
  private final Handler mAreaMilestoneHandler = new Handler(Looper.getMainLooper());
  private final Runnable mAcknowledgeAreaMilestone = this::acknowledgeAreaMilestonePresentation;
  private boolean mCompletionCardDebugPreview;
  private long mCompletionCardGeneratedOsmId;
  private final Observer<Integer> mTopButtonMarginObserver = this::updateTopButtonsMargin;

  private LeftButton mLeftButton;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    final FragmentActivity activity = requireActivity();
    mMapButtonClickListener = (MwmActivity) activity;
    mPlacePageViewModel = new ViewModelProvider(activity).get(PlacePageViewModel.class);
    mMapButtonsViewModel = new ViewModelProvider(activity).get(MapButtonsViewModel.class);
    final LayoutMode layoutMode = mMapButtonsViewModel.getLayoutMode().getValue();
    if (layoutMode == LayoutMode.navigation)
      mFrame = inflater.inflate(R.layout.map_buttons_layout_navigation, container, false);
    else if (layoutMode == LayoutMode.planning)
      mFrame = inflater.inflate(R.layout.map_buttons_layout_planning, container, false);
    else
      mFrame = inflater.inflate(R.layout.map_buttons_layout_regular, container, false);

    mInnerLeftButtonsFrame = mFrame.findViewById(R.id.map_buttons_inner_left);
    mInnerRightButtonsFrame = mFrame.findViewById(R.id.map_buttons_inner_right);
    mBottomButtonsFrame = mFrame.findViewById(R.id.map_buttons_bottom);

    mButtonsMap = new HashMap<>();

    initBottomButtons();

    final View zoomFrame = mFrame.findViewById(R.id.zoom_buttons_container);
    mFrame.findViewById(R.id.nav_zoom_in)
        .setOnClickListener((v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.zoomIn));
    mFrame.findViewById(R.id.nav_zoom_out)
        .setOnClickListener((v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.zoomOut));
    final View myPosition = mFrame.findViewById(R.id.my_position);
    mNavMyPosition =
        new MyPositionButton(myPosition, (v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.myPosition));

    // Some buttons do not exist in navigation mode
    mToggleMapLayerButton = mFrame.findViewById(R.id.layers_button);
    if (mToggleMapLayerButton != null)
    {
      mToggleMapLayerButton.setOnClickListener(
          view -> mMapButtonClickListener.onMapButtonClick(MapButtons.toggleMapLayer));
      mToggleMapLayerButton.setVisibility(View.VISIBLE);
    }
    mMapButtonsViewModel.setTopButtonsMarginTop(-1);

    mTrackRecordingStatusButton = mFrame.findViewById(R.id.track_recording_status);
    if (mTrackRecordingStatusButton != null)
      mTrackRecordingStatusButton.setOnClickListener(
          view -> mMapButtonClickListener.onMapButtonClick(MapButtons.trackRecordingStatus));

    mSearchWheel = new SearchWheel(mFrame,
                                   (v)
                                       -> mMapButtonClickListener.onMapButtonClick(MapButtons.search),
                                   (v) -> mMapButtonClickListener.onSearchCanceled(), mMapButtonsViewModel);

    // Used to get the maximum height the buttons will evolve in
    mFrame.addOnLayoutChangeListener(new MapButtonsController.ContentViewLayoutChangeListener(mFrame));

    mButtonsMap.put(MapButtons.zoom, zoomFrame);
    mButtonsMap.put(MapButtons.myPosition, myPosition);

    if (mToggleMapLayerButton != null)
      mButtonsMap.put(MapButtons.toggleMapLayer, mToggleMapLayerButton);
    if (mTrackRecordingStatusButton != null)
      mButtonsMap.put(MapButtons.trackRecordingStatus, mTrackRecordingStatusButton);
    showButton(false, MapButtons.trackRecordingStatus);
    return mFrame;
  }

  private void initBottomButtons()
  {
    // universal button
    applyLeftButton();

    // bookmarks button
    View bookmarksButton = mFrame.findViewById(R.id.btn_bookmarks);
    if (bookmarksButton != null)
    {
      bookmarksButton.setOnClickListener((v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.bookmarks));
      mButtonsMap.put(MapButtons.bookmarks, bookmarksButton);
    }

    // search button
    View searchButton = mFrame.findViewById(R.id.btn_search);
    if (searchButton != null)
    {
      searchButton.setOnClickListener((v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.bookmarks));
      mButtonsMap.put(MapButtons.search, searchButton);
    }

    // menu button
    View menuButton = mFrame.findViewById(R.id.menu_button);
    if (menuButton != null)
    {
      menuButton.setOnClickListener((v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.menu));
      // This hack is needed to show the badge on the initial startup. For some reason, updateMenuBadge does not work
      // from onResume() there.
      menuButton.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
        @Override
        public void onGlobalLayout()
        {
          updateMenuBadge();
          menuButton.getViewTreeObserver().removeOnGlobalLayoutListener(this);
        }
      });
      mButtonsMap.put(MapButtons.menu, menuButton);
    }

    mExplorationBadge = mFrame.findViewById(R.id.exploration_percentage);
    if (mExplorationBadge != null)
    {
      mButtonsMap.put(MapButtons.explorationBanner, mExplorationBadge);
      mExplorationBadge.setOnClickListener(v -> {
        Context ctx = getContext();
        if (ctx == null)
          return;
        FocusedAreaProgress progress =
            MwmApplication.from(ctx).getStreetPixelsManager().getFocusedAreaProgress();
        Log.i("StreetPixels",
              "sheet open hasFocus=" + progress.hasFocus + " fractionValid=" + progress.fractionValid
                  + " citySummary=" + progress.citySummary + " compactIndex=" + progress.compactIndex
                  + " fraction=" + progress.fraction + " areaCompleted=" + progress.areaCompleted
                  + " noExplorationArea=" + progress.noExplorationArea);
        if (progress.noExplorationArea || !progress.hasFocus || TextUtils.isEmpty(progress.displayName))
        {
          FocusedAreaDetailBottomSheet.showEmpty(getParentFragmentManager());
          return;
        }
        FocusedAreaDetailBottomSheet.show(getParentFragmentManager(), progress.displayName, progress.fractionValid,
                                          progress.fraction, progress.areaCompleted, progress.previouslyCompleted,
                                          progress.osmId, progress.citySummary);
      });
    }
    mGpsWaitingBadge = mFrame.findViewById(R.id.gps_waiting_badge);
    if (mGpsWaitingBadge != null)
    {
      mButtonsMap.put(MapButtons.gpsWaitingBanner, mGpsWaitingBadge);
      showButton(false, MapButtons.gpsWaitingBanner);
    }
    mFirstGoalBadge = mFrame.findViewById(R.id.first_goal_badge);
    if (mFirstGoalBadge != null)
    {
      mButtonsMap.put(MapButtons.firstGoalBanner, mFirstGoalBadge);
      showButton(false, MapButtons.firstGoalBanner);
    }
    mCompetitionModeToggle = mFrame.findViewById(R.id.competition_mode_toggle);
    if (mCompetitionModeToggle != null)
    {
      mCompetitionModeToggle.addOnButtonCheckedListener((group, checkedId, isChecked) -> {
        if (mUpdatingCompetitionToggle || !isChecked)
          return;
        int mode = checkedId == R.id.competition_mode_competition ? 1 : 0;
        Framework.nativeSetCompetitionMapMode(mode);
        if (mode == 1)
          fetchCompetitionSnapshotAndMaybeOvertake();
      });
    }
    mCompetitionHintBadge = mFrame.findViewById(R.id.competition_hint_badge);
    if (mCompetitionHintBadge != null)
    {
      mCompetitionHintBadge.setOnClickListener(v -> {
        ExploreConsentDialogFragment.maybeShow(getParentFragmentManager(), new ExploreConsentDialogFragment.Listener()
        {
          @Override
          public void onExploreConsentGranted()
          {
            hideCompetitionHintBadge();
            refreshCompetitionToggle();
          }

          @Override
          public void onExploreConsentDeclined()
          {
            hideCompetitionHintBadge();
          }
        });
      });
      UiUtils.hide(mCompetitionHintBadge);
    }
    mCompletionCard = mFrame.findViewById(R.id.area_completion_card);
    if (mCompletionCard != null)
    {
      mCompletionCardTitle = mCompletionCard.findViewById(R.id.area_completion_card_title);
      mCompletionCardBody = mCompletionCard.findViewById(R.id.area_completion_card_body);
      mCompletionCardOutline = mCompletionCard.findViewById(R.id.area_completion_card_outline);
      mCompletionCardNickname = mCompletionCard.findViewById(R.id.area_completion_card_nickname);
      mCompletionCardDate = mCompletionCard.findViewById(R.id.area_completion_card_date);
      mCompletionCardCompetition = mCompletionCard.findViewById(R.id.area_completion_card_competition);
      mCompletionCardBranding = mCompletionCard.findViewById(R.id.area_completion_card_branding);
      View share = mCompletionCard.findViewById(R.id.area_completion_card_share);
      if (share != null)
        share.setOnClickListener(v -> shareCompletionCard());
    }
  }

  private void applyLeftButton()
  {
    FloatingActionButton leftButtonView = mFrame.findViewById(R.id.left_button);
    if (leftButtonView != null && mLeftButton != null && !mLeftButton.getCode().equals(DISABLE_BUTTON_CODE))
    {
      UiUtils.show(leftButtonView);

      Context context = getContext();
      if (context == null)
        return;

      leftButtonView.setImageTintList(ColorStateList.valueOf(ThemeUtils.getColor(context, R.attr.iconTint)));

      mLeftButton.drawIcon(leftButtonView);
      leftButtonView.setContentDescription(mLeftButton.getPrefsName());
      leftButtonView.setOnClickListener((v) -> mLeftButton.onClick(leftButtonView));
      //      else
      //      {
      //        helpButton.setImageResource(R.drawable.ic_launcher);
      //      }
      //      // Keep this button colorful in normal theme.
      //      if (!ThemeUtils.isNightTheme())
      //        helpButton.getDrawable().setTintList(null);
    }
    else if (leftButtonView != null)
    {
      UiUtils.hide(leftButtonView);
    }
  }

  public void showButton(boolean show, MapButtonsController.MapButtons button)
  {
    // TODO(AB): Why do we need this check? Isn't it better to crash and fix the wrong logic ASAP?
    final View buttonView = mButtonsMap.get(button);
    if (buttonView == null)
      return;
    switch (button)
    {
    case zoom: UiUtils.showIf(show && Config.showZoomButtons(), buttonView); break;
    case toggleMapLayer:
      if (mToggleMapLayerButton != null)
        UiUtils.showIf(show && !isInNavigationMode(), mToggleMapLayerButton);
      break;
    case myPosition:
      if (mNavMyPosition != null)
        mNavMyPosition.showButton(show);
      break;
    case search: mSearchWheel.show(show);
    case bookmarks:
    case menu: UiUtils.showIf(show, buttonView); break;
    case explorationBanner:
      UiUtils.showIf(show, buttonView);
      break;
    case gpsWaitingBanner:
      UiUtils.showIf(show, buttonView);
      break;
    case firstGoalBanner:
      UiUtils.showIf(show, buttonView);
      break;
    case trackRecordingStatus:
      UiUtils.showIf(show, buttonView);
      break;
    }
  }

  private void onRecordingSessionStateChanged(@Nullable Integer stateBoxed)
  {
    final int state = stateBoxed != null ? stateBoxed : RecordingSession.STATE_IDLE;
    final boolean active = RecordingSessionUiModel.isActive(state);
    updateMenuBadge(active);
    showButton(active, MapButtons.trackRecordingStatus);
    updateLeftButtonToggleState(active);
    updateTrackRecordingStatusAppearance(state);
    refreshFirstGoalBadge();
    refreshGpsWaitingBadge();
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    mGpsTimedOut = false;
    refreshGpsWaitingBadge();
  }

  @Override
  public void onLocationUpdateTimeout()
  {
    mGpsTimedOut = true;
    refreshGpsWaitingBadge();
  }

  private void refreshGpsWaitingBadge()
  {
    if (mGpsWaitingBadge == null)
      return;
    Context ctx = getContext();
    if (ctx == null)
    {
      showButton(false, MapButtons.gpsWaitingBanner);
      return;
    }
    int state = RecordingSession.getState();
    boolean active = RecordingSession.isActive(state);
    boolean paused = state == RecordingSession.STATE_PAUSED;
    Location location = MwmApplication.from(ctx).getLocationHelper().getSavedLocation();
    boolean hasLocation = location != null && !mGpsTimedOut;
    boolean hasAccuracy = hasLocation && location.hasAccuracy();
    float accuracy = hasAccuracy ? location.getAccuracy() : 0.0f;
    showButton(GpsWaitingState.showWaiting(active, paused, hasLocation, hasAccuracy, accuracy),
               MapButtons.gpsWaitingBanner);
  }

  private void updateTrackRecordingStatusAppearance(@RecordingSession.State int state)
  {
    if (mTrackRecordingStatusButton == null)
      return;

    stopTrackRecordingBlink();
    final Context context = getContext();
    if (context == null)
      return;

    if (state == RecordingSession.STATE_RECORDING)
    {
      mTrackRecordingStatusButton.setImageTintList(null);
      mTrackRecordingStatusButton.setContentDescription(getString(R.string.track_recording_alert_title));
      animateIconBlinking(mTrackRecordingStatusButton);
    }
    else if (state == RecordingSession.STATE_PAUSED)
    {
      mTrackRecordingStatusButton.setContentDescription(getString(R.string.track_recording_alert_title));
      mTrackRecordingStatusButton.setImageTintList(
          ColorStateList.valueOf(ContextCompat.getColor(context, R.color.active_track_recording)));
    }
  }

  void animateIconBlinking(@NonNull FloatingActionButton button)
  {
    Drawable drawable = button.getDrawable();
    mTrackRecordingBlinkAnimator = ObjectAnimator.ofArgb(drawable, "tint", 0xFF757575, 0xFFFF0000);
    mTrackRecordingBlinkAnimator.setDuration(2500);
    mTrackRecordingBlinkAnimator.setEvaluator(new ArgbEvaluator());
    mTrackRecordingBlinkAnimator.setRepeatCount(ObjectAnimator.INFINITE);
    mTrackRecordingBlinkAnimator.setRepeatMode(ObjectAnimator.REVERSE);
    mTrackRecordingBlinkAnimator.start();
  }

  private void stopTrackRecordingBlink()
  {
    if (mTrackRecordingBlinkAnimator != null)
    {
      mTrackRecordingBlinkAnimator.cancel();
      mTrackRecordingBlinkAnimator = null;
    }
    if (mTrackRecordingStatusButton != null)
      mTrackRecordingStatusButton.clearColorFilter();
  }

  private static int dpToPx(float dp, Context context)
  {
    return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, context.getResources().getDisplayMetrics());
  }

  private void updateTopButtonsMargin(int margin)
  {
    if (margin == -1 || mTrackRecordingStatusButton == null)
      return;
    ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) mTrackRecordingStatusButton.getLayoutParams();
    params.topMargin = margin;
    mTrackRecordingStatusButton.setLayoutParams(params);
  }

  @OptIn(markerClass = ExperimentalBadgeUtils.class)
  private void updateMenuBadge(Boolean enable)
  {
    final View menuButton = mButtonsMap.get(MapButtons.menu);
    final Context context = getContext();
    // Sometimes the global layout listener fires when the fragment is not attached to a context
    if (menuButton == null || context == null)
      return;
    final UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
    final int count = (info == null ? 0 : info.filesCount);
    final int verticalOffset = dpToPx(8, context) + dpToPx(Integer.toString(0).length() * 5, context);

    if (count == 0)
    {
      BadgeUtils.detachBadgeDrawable(mBadgeDrawable, menuButton);
      mBadgeDrawable = BadgeDrawable.create(context);
      mBadgeDrawable.setMaxCharacterCount(0);
      mBadgeDrawable.setHorizontalOffset(verticalOffset);
      mBadgeDrawable.setVerticalOffset(dpToPx(9, context));
      mBadgeDrawable.setBackgroundColor(ContextCompat.getColor(context, R.color.active_track_recording));
      mBadgeDrawable.setVisible(enable);
      BadgeUtils.attachBadgeDrawable(mBadgeDrawable, menuButton);
    }
  }

  @OptIn(markerClass = com.google.android.material.badge.ExperimentalBadgeUtils.class)
  public void updateMenuBadge()
  {
    final View menuButton = mButtonsMap.get(MapButtons.menu);
    final Context context = getContext();
    // Sometimes the global layout listener fires when the fragment is not attached to a context
    if (menuButton == null || context == null)
      return;
    final UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
    final int count = (info == null ? 0 : info.filesCount);
    final int verticalOffset = dpToPx(8, context) + dpToPx(Integer.toString(0).length() * 5, context);
    BadgeUtils.detachBadgeDrawable(mBadgeDrawable, menuButton);
    mBadgeDrawable = BadgeDrawable.create(context);
    mBadgeDrawable.setMaxCharacterCount(3);
    mBadgeDrawable.setHorizontalOffset(verticalOffset);
    mBadgeDrawable.setVerticalOffset(dpToPx(9, context));
    mBadgeDrawable.setNumber(count);
    mBadgeDrawable.setVisible(count > 0);
    BadgeUtils.attachBadgeDrawable(mBadgeDrawable, menuButton);

    updateMenuBadge(RecordingSession.isActive());
  }

  public void updateLayerButton()
  {
    if (mToggleMapLayerButton == null)
      return;
    final boolean buttonSelected = TrafficManager.INSTANCE.isEnabled() || IsolinesManager.isEnabled()
                                || SubwayManager.isEnabled() || Framework.nativeIsOutdoorsLayerEnabled()
                                || StreetPixelsManager.isEnabled()
                                || StreetPixelsManager.areExplorationAreasEnabled();
    mToggleMapLayerButton.setHasActiveLayers(buttonSelected);
  }

  private void updateExplorationBadge(@Nullable StreetPixelsState state)
  {
    if (state == null)
      return;

    if (state.getStatus() == StreetPixelsState.Status.LOADING)
    {
      mExplorationBadge.setText("Loading exploration progress...");
      showButton(true, MapButtons.explorationBanner);
    }
    else if (state.getStatus() == StreetPixelsState.Status.READY)
    {
      Log.i("MapButtonsController", "updateExplorationBadge: READY");
      Context ctx = getContext();
      if (ctx != null)
      {
        String countryId = state.getCountryId();
        FocusedAreaProgress progress =
            MwmApplication.from(ctx).getStreetPixelsManager().refreshFocusedAreaAtMapCenter(
                countryId != null ? countryId : "");
        applyExplorationBadge(progress);
      }
      else
      {
        Log.i("MapButtonsController", "updateExplorationBadge: CONTEXT IS NULL");
      }
    }
    else
    {
      Log.i("MapButtonsController", "updateExplorationBadge: NOT_READY");
      showButton(false, MapButtons.explorationBanner);
    }
  }

  private void bindExplorationBadgeFromProgress(@NonNull FocusedAreaProgress progress)
  {
    StreetPixelsState state = mMapButtonsViewModel.getStreetPixelsState().getValue();
    if (state != null && state.getStatus() == StreetPixelsState.Status.LOADING)
      return;
    if (state != null && state.getStatus() == StreetPixelsState.Status.NOT_READY)
    {
      showButton(false, MapButtons.explorationBanner);
      return;
    }
    applyExplorationBadge(progress);
  }

  private void applyExplorationBadge(@NonNull FocusedAreaProgress progress)
  {
    Context ctx = getContext();
    if (ctx == null || mExplorationBadge == null)
      return;
    if (progress.hasFocus && !TextUtils.isEmpty(progress.displayName))
    {
      Log.i("StreetPixels",
            "badge hasFocus=" + progress.hasFocus + " fractionValid=" + progress.fractionValid
                + " citySummary=" + progress.citySummary + " compactIndex=" + progress.compactIndex
                + " fraction=" + progress.fraction + " areaCompleted=" + progress.areaCompleted
                + " omitPercent=" + !progress.fractionValid);
      if (progress.fractionValid)
      {
        if (progress.areaCompleted)
        {
          mExplorationBadge.setText(progress.displayName + " • " +
                                    ctx.getString(R.string.street_pixels_area_completed));
        }
        else
        {
          mExplorationBadge.setText(progress.displayName + " • " +
                                    FocusedAreaProgress.formatPercent(progress.fraction));
        }
      }
      else
      {
        mExplorationBadge.setText(progress.displayName);
      }
      showButton(true, MapButtons.explorationBanner);
    }
    else
    {
      mExplorationBadge.setText(R.string.street_pixels_no_exploration_area);
      showButton(true, MapButtons.explorationBanner);
    }
    refreshCompetitionToggle();
    onCompetitionHintReady();
  }

  private void refreshFirstGoalBadge()
  {
    Context ctx = getContext();
    if (ctx == null)
      return;
    applyFirstGoalBadge(MwmApplication.from(ctx).getStreetPixelsManager().getFirstGoalProgress());
  }

  private void applyFirstGoalBadge(@NonNull FirstGoalProgress progress)
  {
    if (mFirstGoalBadge == null)
      return;
    if (progress.state == FirstGoalProgress.STATE_IN_PROGRESS)
    {
      mFirstGoalBadge.animate().cancel();
      mFirstGoalBadge.setAlpha(1f);
      mFirstGoalBadge.setText(
          getString(R.string.street_pixels_first_goal_progress, progress.collected, progress.threshold));
      showButton(true, MapButtons.firstGoalBanner);
      mFirstGoalBadge.extend();
    }
    else if (progress.state == FirstGoalProgress.STATE_COMPLETE && mFirstGoalBadge.getVisibility() == View.VISIBLE)
    {
      mFirstGoalBadge.setText(
          getString(R.string.street_pixels_first_goal_progress, progress.threshold, progress.threshold));
      mFirstGoalBadge.animate().alpha(0f).setDuration(250).withEndAction(() -> {
        if (!isAdded() || mFirstGoalBadge == null)
          return;
        showButton(false, MapButtons.firstGoalBanner);
        mFirstGoalBadge.setAlpha(1f);
      }).start();
    }
    else
    {
      mFirstGoalBadge.animate().cancel();
      showButton(false, MapButtons.firstGoalBanner);
    }
  }

  private void refreshCompetitionToggle()
  {
    if (mCompetitionModeToggle == null)
      return;
    boolean show = Framework.nativeHasExploreConsent();
    UiUtils.showIf(show, mCompetitionModeToggle);
    if (!show)
      return;
    mUpdatingCompetitionToggle = true;
    int mode = Framework.nativeGetCompetitionMapMode();
    mCompetitionModeToggle.check(mode == 1 ? R.id.competition_mode_competition : R.id.competition_mode_explore);
    mUpdatingCompetitionToggle = false;
  }

  private void fetchCompetitionSnapshotAndMaybeOvertake()
  {
    Context ctx = getContext();
    if (ctx == null)
      return;
    StreetPixelsManager manager = MwmApplication.from(ctx).getStreetPixelsManager();
    FocusedAreaProgress progress = manager.getFocusedAreaProgress();
    if (progress.hasFocus && progress.osmId != 0 && Framework.nativeHasExploreConsent())
    {
      manager.requestCompetitionAreaSnapshot(progress.osmId, chrome -> {
        if (getContext() == null)
          return;
        maybeShowOvertakingHint(manager);
      });
      return;
    }
    maybeShowOvertakingHint(manager);
  }

  private void maybeShowOvertakingHint(@NonNull StreetPixelsManager manager)
  {
    Context ctx = getContext();
    if (ctx == null)
      return;
    String text = manager.tryConsumeOvertakingHint(Framework.nativeIsRoutingFollowing());
    if (text == null || text.isEmpty())
      return;
    Toast.makeText(ctx, text, Toast.LENGTH_LONG).show();
  }

  private void onCompetitionHintReady()
  {
    Context ctx = getContext();
    if (ctx == null || mCompetitionHintBadge == null)
      return;
    if (Framework.nativeIsRoutingFollowing() || Framework.nativeHasExploreConsent())
      return;
    if (mCompetitionHintBadge.getVisibility() == View.VISIBLE)
      return;
    StreetPixelsManager manager = MwmApplication.from(ctx).getStreetPixelsManager();
    String text = manager.peekCompetitionHintText();
    if (text == null || text.isEmpty())
      return;
    mCompetitionHintBadge.setText(text);
    UiUtils.show(mCompetitionHintBadge);
    mCompetitionHintBadge.extend();
    manager.acknowledgeCompetitionHint();
    mCompetitionHintHandler.removeCallbacks(mHideCompetitionHint);
    mCompetitionHintHandler.postDelayed(mHideCompetitionHint, 4000);
  }

  private void hideCompetitionHintBadge()
  {
    mCompetitionHintHandler.removeCallbacks(mHideCompetitionHint);
    if (mCompetitionHintBadge != null)
      UiUtils.hide(mCompetitionHintBadge);
  }

  private void refreshAreaMilestonePresentation()
  {
    Context ctx = getContext();
    if (ctx == null)
      return;
    applyAreaMilestonePresentation(
        MwmApplication.from(ctx).getStreetPixelsManager().getCurrentAreaMilestonePresentation());
  }

  private void applyAreaMilestonePresentation(@Nullable AreaMilestonePresentation presentation)
  {
    Context ctx = getContext();
    mAreaMilestoneHandler.removeCallbacks(mAcknowledgeAreaMilestone);
    if (mCompletionCard != null)
      UiUtils.hide(mCompletionCard);
    mCompletionCardDebugPreview = presentation != null && presentation.debugPreview;
    if (presentation == null || presentation.threshold != AreaMilestonePresentation.THRESHOLD_100)
      mCompletionCardGeneratedOsmId = 0;
    if (ctx == null || presentation == null)
      return;
    String name = presentation.displayName;
    int messageId = R.string.street_pixels_area_milestone_25;
    int toastLength = Toast.LENGTH_SHORT;
    long delayMs = 2000;
    if (presentation.threshold == AreaMilestonePresentation.THRESHOLD_50)
    {
      messageId = R.string.street_pixels_area_milestone_50;
      toastLength = Toast.LENGTH_LONG;
      delayMs = 3500;
    }
    else if (presentation.threshold == AreaMilestonePresentation.THRESHOLD_100)
    {
      messageId = R.string.street_pixels_area_milestone_100;
      toastLength = Toast.LENGTH_LONG;
      delayMs = 4000;
      if (mCompletionCardTitle != null)
        mCompletionCardTitle.setText(getString(R.string.street_pixels_area_milestone_100, name));
      if (mCompletionCardBody != null)
        mCompletionCardBody.setText(getString(R.string.street_pixels_completion_card_body, name));
      boolean recordGenerated = CompletionCardGeneratedGate.shouldRecord(
          mCompletionCardDebugPreview, presentation.threshold, presentation.osmId, mCompletionCardGeneratedOsmId);
      bindCompletionCardOutline(MwmApplication.from(ctx).getStreetPixelsManager().getCurrentCompletionCard(
          recordGenerated));
      if (recordGenerated)
        mCompletionCardGeneratedOsmId = presentation.osmId;
      if (mCompletionCard != null)
        UiUtils.show(mCompletionCard);
    }
    if (!mCompletionCardDebugPreview)
    {
      Toast.makeText(ctx, getString(messageId, name), toastLength).show();
      pulseExplorationBadge(presentation.threshold);
      mAreaMilestoneHandler.postDelayed(mAcknowledgeAreaMilestone, delayMs);
    }
  }

  private void bindCompletionCardOutline(@Nullable CompletionCardModel card)
  {
    boolean hasOutline = card != null && card.outlineXs != null && card.outlineXs.length > 0;
    if (mCompletionCardOutline != null)
    {
      if (hasOutline)
      {
        mCompletionCardOutline.setOutline(card.outlineXs, card.outlineYs, card.ringLengths);
        mCompletionCardOutline.setContentDescription(card.areaDisplayName);
        UiUtils.show(mCompletionCardOutline);
      }
      else
      {
        mCompletionCardOutline.setOutline(null, null, null);
        UiUtils.hide(mCompletionCardOutline);
      }
    }
    if (mCompletionCardBranding != null)
    {
      if (card != null && !TextUtils.isEmpty(card.branding))
      {
        mCompletionCardBranding.setText(card.branding);
        UiUtils.show(mCompletionCardBranding);
      }
      else
        UiUtils.hide(mCompletionCardBranding);
    }
    if (mCompletionCardNickname != null)
    {
      boolean showNick = card != null && !TextUtils.isEmpty(card.nickname);
      if (showNick)
        mCompletionCardNickname.setText(card.nickname);
      UiUtils.showIf(showNick, mCompletionCardNickname);
    }
    if (mCompletionCardDate != null)
    {
      boolean showDate = card != null && !TextUtils.isEmpty(card.completedDate);
      if (showDate)
        mCompletionCardDate.setText(card.completedDate);
      UiUtils.showIf(showDate, mCompletionCardDate);
    }
    if (mCompletionCardCompetition != null)
    {
      boolean showCompetition = card != null && !TextUtils.isEmpty(card.competitionLine);
      if (showCompetition)
        mCompletionCardCompetition.setText(card.competitionLine);
      UiUtils.showIf(showCompetition, mCompletionCardCompetition);
    }
  }

  private void shareCompletionCard()
  {
    Context ctx = getContext();
    if (ctx == null)
      return;
    StreetPixelsManager manager = MwmApplication.from(ctx).getStreetPixelsManager();
    CompletionCardSharePayload payload = manager.prepareCompletionCardShare();
    if (payload == null || TextUtils.isEmpty(payload.path) || !"image/png".equals(payload.mimeType))
      return;
    if (!mCompletionCardDebugPreview)
      manager.recordCompletionCardShareInitiated();
    CompletionCardShare.shareImage(ctx, payload);
  }

  private void pulseExplorationBadge(int threshold)
  {
    if (mExplorationBadge == null)
      return;
    float scale = threshold == AreaMilestonePresentation.THRESHOLD_25 ? 1.08f : 1.16f;
    long duration = threshold == AreaMilestonePresentation.THRESHOLD_25 ? 200 : 400;
    mExplorationBadge.animate().cancel();
    mExplorationBadge.setScaleX(1f);
    mExplorationBadge.setScaleY(1f);
    mExplorationBadge.animate()
        .scaleX(scale)
        .scaleY(scale)
        .setDuration(duration)
        .withEndAction(() -> {
          if (mExplorationBadge == null)
            return;
          mExplorationBadge.animate().scaleX(1f).scaleY(1f).setDuration(duration).start();
        })
        .start();
  }

  private void acknowledgeAreaMilestonePresentation()
  {
    Context ctx = getContext();
    if (mCompletionCard != null)
      UiUtils.hide(mCompletionCard);
    if (ctx == null)
      return;
    MwmApplication.from(ctx).getStreetPixelsManager().acknowledgeAreaMilestonePresentation();
  }

  private boolean isBehindPlacePage(View v)
  {
    if (mPlacePageViewModel == null)
      return false;
    final Integer placePageWidth = mPlacePageViewModel.getPlacePageWidth().getValue();
    if (placePageWidth != null)
      return !(mContentWidth / 2 > (placePageWidth.floatValue() / 2.0) + v.getWidth());
    return true;
  }

  private boolean isMoving(View v)
  {
    return v.getTranslationY() < 0;
  }

  public void move(float translationY)
  {
    if (mContentHeight == 0)
      return;

    // Move the buttons containers to follow the place page
    if (mInnerRightButtonsFrame != null
        && (isBehindPlacePage(mInnerRightButtonsFrame) || isMoving(mInnerRightButtonsFrame)))
      applyMove(mInnerRightButtonsFrame, translationY);
    if (mInnerLeftButtonsFrame != null
        && (isBehindPlacePage(mInnerLeftButtonsFrame) || isMoving(mInnerLeftButtonsFrame)))
      applyMove(mInnerLeftButtonsFrame, translationY);
  }

  private void applyMove(View frame, float translationY)
  {
    final float rightTranslation = translationY - frame.getBottom();
    final float appliedTranslation = rightTranslation <= 0 ? rightTranslation : 0;
    frame.setTranslationY(appliedTranslation);
    updateButtonsVisibility(appliedTranslation, frame);
  }

  public void updateButtonsVisibility()
  {
    if (mInnerLeftButtonsFrame != null)
      updateButtonsVisibility(mInnerLeftButtonsFrame.getTranslationY(), mInnerLeftButtonsFrame);
    if (mInnerRightButtonsFrame != null)
      updateButtonsVisibility(mInnerRightButtonsFrame.getTranslationY(), mInnerRightButtonsFrame);
  }

  private void updateButtonsVisibility(final float translation, @Nullable View parent)
  {
    if (parent == null)
      return;
    for (Map.Entry<MapButtons, View> entry : mButtonsMap.entrySet())
    {
      final View button = entry.getValue();
      if (button.getParent() == parent)
      {
        int toleranceOffset = switch (entry.getKey())
        {
          case zoomIn, zoomOut, zoom -> -140;
          default ->
            0;
            // Allow offset tolerance for zoom buttons
        };
        showButton(getViewTopOffset(translation, button) >= toleranceOffset, entry.getKey());
      }
    }
  }

  private float getBottomButtonsHeight()
  {
    if (mBottomButtonsFrame != null && mFrame != null && UiUtils.isVisible(mFrame))
      return mBottomButtonsFrame.getMeasuredHeight();
    else
      return 0;
  }

  public void setButtonsHidden(boolean buttonHidden)
  {
    UiUtils.showIf(!buttonHidden, mFrame);
    if (!buttonHidden)
      updateButtonsVisibility();
    mMapButtonsViewModel.setBottomButtonsHeight(getBottomButtonsHeight());
  }

  private boolean isInNavigationMode()
  {
    return RoutingController.get().isPlanning() || RoutingController.get().isNavigating();
  }

  public void updateNavMyPositionButton(int newMode)
  {
    if (mNavMyPosition != null)
      mNavMyPosition.update(newMode);
  }

  private int getViewTopOffset(float translation, View v)
  {
    return (int) (translation + v.getTop());
  }

  @Override
  public void onStart()
  {
    super.onStart();
    final FragmentActivity activity = requireActivity();
    mPlacePageViewModel.getPlacePageDistanceToTop().observe(activity, mPlacePageDistanceToTopObserver);
    mMapButtonsViewModel.getButtonsHidden().observe(activity, mButtonHiddenObserver);
    mMapButtonsViewModel.getMyPositionMode().observe(activity, mMyPositionModeObserver);
    mMapButtonsViewModel.getSearchOption().observe(activity, mSearchOptionObserver);
    mMapButtonsViewModel.getRecordingSessionState().observe(activity, mRecordingSessionObserver);
    mMapButtonsViewModel.getStreetPixelsState().observe(activity, mStreetPixelsStateObserver);
    StreetPixelsManager.registerFocusedAreaProgressCallback(mFocusedAreaProgressCallback);
    StreetPixelsManager.registerFirstGoalProgressCallback(mFirstGoalProgressCallback);
    StreetPixelsManager.registerAreaMilestonePresentationCallback(mAreaMilestonePresentationCallback);
    StreetPixelsManager.registerCompetitionHintCallback(mCompetitionHintCallback);
    getParentFragmentManager().setFragmentResultListener(MyAccountDialogFragment.RESULT_COMPETITION_ACCOUNT, this,
                                                         (key, bundle) -> refreshCompetitionToggle());
    mMapButtonsViewModel.getTopButtonsMarginTop().observe(activity, mTopButtonMarginObserver);
    MwmApplication.from(activity).getLocationHelper().addListener(this);
  }

  public void onResume()
  {
    super.onResume();
    mSearchWheel.onResume();
    updateMenuBadge();
    updateLayerButton();

    @Nullable StreetPixelsState state = mMapButtonsViewModel.getStreetPixelsState().getValue();
    updateExplorationBadge(state);
    refreshFirstGoalBadge();
    refreshGpsWaitingBadge();
    refreshAreaMilestonePresentation();
    Context ctx = getContext();
    if (ctx != null)
      MwmApplication.from(ctx).getStreetPixelsManager().releaseCompletionCardShare();
    refreshCompetitionToggle();
    onCompetitionHintReady();

    final WindowInsetUtils.PaddingInsetsListener insetsListener =
        new WindowInsetUtils.PaddingInsetsListener.Builder()
            .setInsetsTypeMask(WindowInsetsCompat.Type.systemBars() | WindowInsetsCompat.Type.displayCutout())
            .setAllSides()
            .build();
    ViewCompat.setOnApplyWindowInsetsListener(mFrame, insetsListener);
    // Fixes insets on older Androids and with a search opened via API on all Androids.
    if (mFrame.hasWindowFocus())
      ViewCompat.requestApplyInsets(mFrame);
  }

  @Override
  public void onPause()
  {
    ViewCompat.setOnApplyWindowInsetsListener(mFrame, null);
    super.onPause();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mMapButtonsViewModel.getTopButtonsMarginTop().removeObserver(mTopButtonMarginObserver);
    mPlacePageViewModel.getPlacePageDistanceToTop().removeObserver(mPlacePageDistanceToTopObserver);
    mMapButtonsViewModel.getButtonsHidden().removeObserver(mButtonHiddenObserver);
    mMapButtonsViewModel.getMyPositionMode().removeObserver(mMyPositionModeObserver);
    mMapButtonsViewModel.getSearchOption().removeObserver(mSearchOptionObserver);
    mMapButtonsViewModel.getRecordingSessionState().removeObserver(mRecordingSessionObserver);
    StreetPixelsManager.unregisterFocusedAreaProgressCallback(mFocusedAreaProgressCallback);
    StreetPixelsManager.unregisterFirstGoalProgressCallback(mFirstGoalProgressCallback);
    StreetPixelsManager.unregisterAreaMilestonePresentationCallback(mAreaMilestonePresentationCallback);
    StreetPixelsManager.unregisterCompetitionHintCallback(mCompetitionHintCallback);
    mCompetitionHintHandler.removeCallbacks(mHideCompetitionHint);
    mAreaMilestoneHandler.removeCallbacks(mAcknowledgeAreaMilestone);
    if (mFirstGoalBadge != null)
      mFirstGoalBadge.animate().cancel();
    if (mExplorationBadge != null)
      mExplorationBadge.animate().cancel();
    mMapButtonsViewModel.getStreetPixelsState().removeObserver(mStreetPixelsStateObserver);
    MwmApplication.from(requireActivity()).getLocationHelper().removeListener(this);
  }

  public void onSearchOptionChange(@Nullable SearchWheel.SearchOption searchOption)
  {
    if (searchOption == null)
      mSearchWheel.reset();
  }

  public void setLeftButton(LeftButton leftButton)
  {
    this.mLeftButton = leftButton;
  }

  public void reloadLeftButton(LeftButton leftButton)
  {
    setLeftButton(leftButton);
    applyLeftButton();
  }

  private void updateLeftButtonToggleState(boolean isEnabled)
  {
    if (mLeftButton instanceof LeftToggleButton)
    {
      ((LeftToggleButton) mLeftButton).setChecked(isEnabled);

      reloadLeftButton(mLeftButton);
    }
  }

  public enum LayoutMode
  {
    regular,
    planning,
    navigation
  }

  public enum MapButtons
  {
    myPosition,
    toggleMapLayer,
    zoomIn,
    zoomOut,
    zoom,
    search,
    bookmarks,
    menu,
    help,
    explorationBanner,
    gpsWaitingBanner,
    firstGoalBanner,
    trackRecordingStatus
  }

  public interface MapButtonClickListener
  {
    void onMapButtonClick(MapButtons button);

    void onSearchCanceled();
  }

  private class ContentViewLayoutChangeListener implements View.OnLayoutChangeListener
  {
    @NonNull
    private final View mContentView;

    public ContentViewLayoutChangeListener(@NonNull View contentView)
    {
      mContentView = contentView;
    }

    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight,
                               int oldBottom)
    {
      mContentHeight = bottom - top;
      mContentWidth = right - left;
      mMapButtonsViewModel.setBottomButtonsHeight(getBottomButtonsHeight());
      mContentView.removeOnLayoutChangeListener(this);
    }
  }
}
