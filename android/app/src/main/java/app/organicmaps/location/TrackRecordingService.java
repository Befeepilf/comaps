package app.organicmaps.location;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.Manifest.permission.POST_NOTIFICATIONS;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

import android.app.Activity;
import android.app.ForegroundServiceStartNotAllowedException;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.location.Location;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.app.ActivityCompat;
import androidx.core.app.NotificationChannelCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.app.ServiceCompat;
import androidx.core.content.ContextCompat;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.ProductAnalytics;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.location.RecordingSession;
import app.organicmaps.sdk.location.TrackRecorder;
import app.organicmaps.sdk.util.LocationUtils;
import app.organicmaps.sdk.util.log.Logger;

public class TrackRecordingService extends Service implements LocationListener
{
  public static final String TRACK_REC_CHANNEL_ID = "TRACK RECORDING";
  public static final String STOP_TRACK_RECORDING = "STOP_TRACK_RECORDING";
  public static final String PAUSE_TRACK_RECORDING = "PAUSE_TRACK_RECORDING";
  public static final String RESUME_TRACK_RECORDING = "RESUME_TRACK_RECORDING";
  public static final int TRACK_REC_NOTIFICATION_ID = 54321;
  private static final String TAG = TrackRecordingService.class.getSimpleName();
  private static final long RECORDING_INTERRUPTION_GAP_MS = 60_000;
  private static final long LOCATION_UPDATE_TIMEOUT_MS = 30_000;
  private static final long RECORDING_INTERRUPTION_FOLLOWUP_MS =
      RECORDING_INTERRUPTION_GAP_MS - LOCATION_UPDATE_TIMEOUT_MS;

  private boolean mWarningNotification = false;
  private boolean mInterruptedNotification = false;
  private boolean mInterruptionLatched = false;
  private final Handler mHandler = new Handler(Looper.getMainLooper());
  private final Runnable mInterruptionFollowupRunnable = this::onRecordingInterruptionFollowup;
  private NotificationCompat.Builder mWarningBuilder;
  private PendingIntent mPendingIntent;
  private PendingIntent mStopPendingIntent;
  private PendingIntent mPausePendingIntent;
  private PendingIntent mResumePendingIntent;

  private final RecordingSession.StateListener mSessionStateListener = (previous, current) -> {
    mHandler.removeCallbacks(mInterruptionFollowupRunnable);
    mInterruptionLatched = false;
    mInterruptedNotification = false;
    updateNotification();
  };

  @Nullable
  @Override
  public IBinder onBind(Intent intent)
  {
    return null;
  }

  @RequiresPermission(value = ACCESS_FINE_LOCATION)
  public static void startForegroundService(@NonNull Context context)
  {
    if (!TrackRecorder.nativeIsTrackRecordingEnabled())
    {
      if (!RecordingSession.isActive())
      {
        Logger.w(TAG, "Refusing to start track recording without an active session");
        return;
      }
      TrackRecorder.nativeStartTrackRecording();
    }
    MwmApplication.from(context).getLocationHelper().restartWithNewMode();
    ContextCompat.startForegroundService(context, new Intent(context, TrackRecordingService.class));
  }

  public static void createNotificationChannel(@NonNull Context context)
  {
    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(context);
    final NotificationChannelCompat channel =
        new NotificationChannelCompat.Builder(TRACK_REC_CHANNEL_ID, NotificationManagerCompat.IMPORTANCE_LOW)
            .setName(context.getString(R.string.track_recording))
            .setLightsEnabled(false)
            .setVibrationEnabled(false)
            .build();
    notificationManager.createNotificationChannel(channel);
  }

  private PendingIntent getContentPendingIntent(@NonNull Context context)
  {
    if (mPendingIntent != null)
      return mPendingIntent;

    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final Intent contentIntent = new Intent(context, MwmActivity.class);
    mPendingIntent =
        PendingIntent.getActivity(context, 0, contentIntent, PendingIntent.FLAG_UPDATE_CURRENT | FLAG_IMMUTABLE);
    return mPendingIntent;
  }

  private PendingIntent getStopPendingIntent(@NonNull Context context)
  {
    if (mStopPendingIntent != null)
      return mStopPendingIntent;

    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final Intent stopIntent = new Intent(context, MwmActivity.class);
    stopIntent.setAction(STOP_TRACK_RECORDING);
    mStopPendingIntent =
        PendingIntent.getActivity(context, 1, stopIntent, PendingIntent.FLAG_UPDATE_CURRENT | FLAG_IMMUTABLE);
    return mStopPendingIntent;
  }

  private PendingIntent getPausePendingIntent(@NonNull Context context)
  {
    if (mPausePendingIntent != null)
      return mPausePendingIntent;

    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final Intent pauseIntent = new Intent(context, TrackRecordingService.class);
    pauseIntent.setAction(PAUSE_TRACK_RECORDING);
    mPausePendingIntent =
        PendingIntent.getService(context, 2, pauseIntent, PendingIntent.FLAG_UPDATE_CURRENT | FLAG_IMMUTABLE);
    return mPausePendingIntent;
  }

  private PendingIntent getResumePendingIntent(@NonNull Context context)
  {
    if (mResumePendingIntent != null)
      return mResumePendingIntent;

    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final Intent resumeIntent = new Intent(context, TrackRecordingService.class);
    resumeIntent.setAction(RESUME_TRACK_RECORDING);
    mResumePendingIntent =
        PendingIntent.getService(context, 3, resumeIntent, PendingIntent.FLAG_UPDATE_CURRENT | FLAG_IMMUTABLE);
    return mResumePendingIntent;
  }

  @NonNull
  public NotificationCompat.Builder buildNotification(@NonNull Context context, @RecordingSession.State int state)
  {
    final RecordingSessionUiModel.NotificationContent content = RecordingSessionUiModel.notificationContent(state);
    final int titleRes = content == RecordingSessionUiModel.NotificationContent.PAUSED
                             ? R.string.track_recording_paused
                             : R.string.track_recording;
    final int textRes = content == RecordingSessionUiModel.NotificationContent.PAUSED
                            ? R.string.track_recording_paused_text
                            : R.string.track_recording_in_progress_text;

    final NotificationCompat.Builder builder =
        new NotificationCompat.Builder(context, TRACK_REC_CHANNEL_ID)
            .setCategory(NotificationCompat.CATEGORY_SERVICE)
            .setPriority(NotificationManager.IMPORTANCE_DEFAULT)
            .setVisibility(NotificationCompat.VISIBILITY_SECRET)
            .setOngoing(true)
            .setShowWhen(true)
            .setOnlyAlertOnce(true)
            .setSmallIcon(R.drawable.ic_logo_small)
            .setContentTitle(context.getString(titleRes))
            .setContentText(context.getString(textRes))
            .setContentIntent(getContentPendingIntent(context))
            .setColor(ContextCompat.getColor(context, R.color.notification));

    for (RecordingSessionUiModel.NotificationAction action : RecordingSessionUiModel.notificationActions(state))
    {
      switch (action)
      {
      case PAUSE:
        builder.addAction(0, context.getString(R.string.pause), getPausePendingIntent(context));
        break;
      case RESUME:
        builder.addAction(0, context.getString(R.string.continue_recording), getResumePendingIntent(context));
        break;
      case STOP:
        builder.addAction(0, context.getString(R.string.navigation_stop_button), getStopPendingIntent(context));
        break;
      }
    }

    return builder;
  }

  public static void stopService(@NonNull Context context)
  {
    Logger.i(TAG);
    context.stopService(new Intent(context, TrackRecordingService.class));
  }

  @Override
  public void onDestroy()
  {
    mHandler.removeCallbacks(mInterruptionFollowupRunnable);
    RecordingSession.unregisterListener(mSessionStateListener);
    mWarningBuilder = null;
    if (TrackRecorder.nativeIsTrackRecordingEnabled())
      TrackRecorder.nativeStopTrackRecording();
    MwmApplication.from(this).getLocationHelper().removeListener(this);
  }

  @Override
  public int onStartCommand(@Nullable Intent intent, int flags, int startId)
  {
    final String action = intent != null ? intent.getAction() : null;
    if (PAUSE_TRACK_RECORDING.equals(action) || RESUME_TRACK_RECORDING.equals(action))
    {
      if (!MwmApplication.from(this).getOrganicMaps().arePlatformAndCoreInitialized())
      {
        Logger.w(TAG, "Application is not initialized");
        stopSelf();
        return START_NOT_STICKY;
      }
      if (PAUSE_TRACK_RECORDING.equals(action))
        RecordingSession.pause();
      else
        RecordingSession.resume();
      mWarningNotification = false;
      updateNotification();
      return START_STICKY;
    }

    if (!MwmApplication.from(this).getOrganicMaps().arePlatformAndCoreInitialized())
    {
      Logger.w(TAG, "Application is not initialized");
      stopSelf();
      return START_NOT_STICKY;
    }

    if (!LocationUtils.checkFineLocationPermission(this))
    {
      Logger.w(TAG, "Permission ACCESS_FINE_LOCATION is not granted, skipping TrackRecordingService");
      stopSelf();
      return START_NOT_STICKY;
    }

    if (!TrackRecorder.nativeIsTrackRecordingEnabled() && !RecordingSession.isActive())
    {
      Logger.i(TAG, "Service can't be started because track recording and session are inactive");
      stopSelf();
      return START_NOT_STICKY;
    }

    Logger.i(TAG, "Starting Track Recording Foreground service");

    try
    {
      int type = 0;
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
        type = ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION;
      ServiceCompat.startForeground(this, TrackRecordingService.TRACK_REC_NOTIFICATION_ID,
                                    buildNotification(this, RecordingSession.getState()).build(), type);
      ProductAnalytics.recordNotifyPermissionGranted();
    }
    catch (Exception e)
    {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && e instanceof ForegroundServiceStartNotAllowedException)
      {
        Logger.e(TAG, "Not in a valid state to start foreground service", e);
      }
      else
        Logger.e(TAG, "Failed to promote the service to foreground", e);
    }

    RecordingSession.registerListener(mSessionStateListener);

    final LocationHelper locationHelper = MwmApplication.from(this).getLocationHelper();
    locationHelper.addListener(this);
    locationHelper.restartWithNewMode();

    return START_STICKY;
  }

  private void updateNotification()
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
        && ActivityCompat.checkSelfPermission(this, POST_NOTIFICATIONS) != PERMISSION_GRANTED)
      return;

    mWarningNotification = false;
    mInterruptedNotification = false;
    NotificationManagerCompat.from(this).notify(TRACK_REC_NOTIFICATION_ID,
                                                buildNotification(this, RecordingSession.getState()).build());
  }

  @NonNull
  private NotificationCompat.Builder buildInterruptedNotification(@NonNull Context context)
  {
    return new NotificationCompat.Builder(context, TRACK_REC_CHANNEL_ID)
        .setCategory(NotificationCompat.CATEGORY_SERVICE)
        .setPriority(NotificationManager.IMPORTANCE_DEFAULT)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setOngoing(true)
        .setShowWhen(true)
        .setOnlyAlertOnce(true)
        .setSmallIcon(R.drawable.ic_logo_small)
        .setContentTitle(context.getString(R.string.track_recording_interrupted))
        .setContentText(context.getString(R.string.track_recording_interrupted_text))
        .setStyle(new NotificationCompat.BigTextStyle().bigText(
            context.getString(R.string.track_recording_interrupted_text)))
        .addAction(0, context.getString(R.string.navigation_stop_button), getStopPendingIntent(context))
        .setContentIntent(getContentPendingIntent(context))
        .setColor(ContextCompat.getColor(context, R.color.notification));
  }

  private void notifyRecordingInterrupted()
  {
    final Activity activity = MwmApplication.from(this).getTopActivity();
    if (activity != null)
    {
      activity.runOnUiThread(
          () -> Toast.makeText(activity, R.string.track_recording_interrupted_text, Toast.LENGTH_LONG).show());
    }

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
        && ActivityCompat.checkSelfPermission(this, POST_NOTIFICATIONS) != PERMISSION_GRANTED)
      return;

    mInterruptedNotification = true;
    mWarningNotification = false;
    NotificationManagerCompat.from(this).notify(TRACK_REC_NOTIFICATION_ID, buildInterruptedNotification(this).build());
  }

  public NotificationCompat.Builder getWarningBuilder(Context context)
  {
    if (mWarningBuilder != null)
      return mWarningBuilder;

    mWarningBuilder =
        new NotificationCompat.Builder(context, TRACK_REC_CHANNEL_ID)
            .setCategory(NotificationCompat.CATEGORY_SERVICE)
            .setPriority(NotificationManager.IMPORTANCE_DEFAULT)
            .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
            .setOngoing(true)
            .setShowWhen(true)
            .setOnlyAlertOnce(true)
            .setSmallIcon(R.drawable.warning_icon)
            .setContentTitle(context.getString(R.string.current_location_unknown_error_title))
            .setContentText(context.getString(R.string.dialog_routing_location_turn_wifi))
            .setStyle(new NotificationCompat.BigTextStyle().bigText(
                context.getString(R.string.dialog_routing_location_turn_wifi)))
            .addAction(0, context.getString(R.string.navigation_stop_button), getStopPendingIntent(context))
            .setContentIntent(getContentPendingIntent(context))
            .setColor(ContextCompat.getColor(context, R.color.notification_warning));

    return mWarningBuilder;
  }

  private void onRecordingInterruptionFollowup()
  {
    if (mInterruptionLatched || RecordingSession.getState() != RecordingSession.STATE_RECORDING)
      return;

    mInterruptionLatched = true;
    RecordingSession.applyInterruptionEffects();
    notifyRecordingInterrupted();
  }

  @Override
  public void onLocationUpdateTimeout()
  {
    Logger.i(TAG, "Location update timeout");

    if (RecordingSession.getState() == RecordingSession.STATE_RECORDING && !mInterruptionLatched)
    {
      mHandler.removeCallbacks(mInterruptionFollowupRunnable);
      mHandler.postDelayed(mInterruptionFollowupRunnable, RECORDING_INTERRUPTION_FOLLOWUP_MS);
    }

    if (mInterruptionLatched)
      return;

    mWarningNotification = true;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
        && ActivityCompat.checkSelfPermission(this, POST_NOTIFICATIONS) != PERMISSION_GRANTED)
      return;

    NotificationManagerCompat.from(this).notify(TRACK_REC_NOTIFICATION_ID, getWarningBuilder(this).build());
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    Logger.i(TAG, "Location is being updated in Track Recording service");

    mHandler.removeCallbacks(mInterruptionFollowupRunnable);
    mInterruptionLatched = false;

    if (mWarningNotification || mInterruptedNotification)
    {
      mWarningNotification = false;
      mInterruptedNotification = false;

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
          && ActivityCompat.checkSelfPermission(this, POST_NOTIFICATIONS) != PERMISSION_GRANTED)
        return;

      NotificationManagerCompat.from(this).notify(TRACK_REC_NOTIFICATION_ID,
                                                  buildNotification(this, RecordingSession.getState()).build());
    }
  }
}
