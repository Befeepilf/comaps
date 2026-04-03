package app.organicmaps.sdk.friends;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.ArrayList;
import java.util.List;

public final class Friends
{
  private Friends() {}

  @Keep
  public static final class Friend
  {
    public final String userId;
    public final String username;
    public Friend(String userId, String username)
    {
      this.userId = userId;
      this.username = username;
    }
  }

  @Keep
  public static final class FriendsPayload
  {
    public final Friend[] accepted;
    public final Friend[] incoming;
    public final Friend[] outgoing;
    public FriendsPayload(Friend[] accepted, Friend[] incoming, Friend[] outgoing)
    {
      this.accepted = accepted;
      this.incoming = incoming;
      this.outgoing = outgoing;
    }
  }

  public interface Callback {
    void onListsUpdated();
    void onSignupResult(boolean success);
    void onUsernameChanged(boolean success);
    void onActionResult(boolean success);
  }

  private static final List<Callback> sCallbacks = new ArrayList<>();

  public static void registerCallback(@NonNull Callback callback)
  {
    synchronized (sCallbacks)
    {
      if (!sCallbacks.contains(callback))
        sCallbacks.add(callback);
      if (sCallbacks.size() == 1)
        nativeSubscribe();
    }
  }

  public static void unregisterCallback(@NonNull Callback callback)
  {
    synchronized (sCallbacks)
    {
      sCallbacks.remove(callback);
      if (sCallbacks.isEmpty())
        nativeUnsubscribe();
    }
  }

  @Keep
  private static void onListsUpdated()
  {
    List<Callback> snapshot;
    synchronized (sCallbacks)
    {
      snapshot = new ArrayList<>(sCallbacks);
    }
    for (Callback cb : snapshot)
      cb.onListsUpdated();
  }

  @Keep
  private static void onSignupResult(boolean success)
  {
    List<Callback> snapshot;
    synchronized (sCallbacks)
    {
      snapshot = new ArrayList<>(sCallbacks);
    }
    for (Callback cb : snapshot)
      cb.onSignupResult(success);
  }

  @Keep
  private static void onUsernameChanged(boolean success)
  {
    List<Callback> snapshot;
    synchronized (sCallbacks)
    {
      snapshot = new ArrayList<>(sCallbacks);
    }
    for (Callback cb : snapshot)
      cb.onUsernameChanged(success);
  }

  @Keep
  private static void onActionResult(boolean success)
  {
    List<Callback> snapshot;
    synchronized (sCallbacks)
    {
      snapshot = new ArrayList<>(sCallbacks);
    }
    for (Callback cb : snapshot)
      cb.onActionResult(success);
  }

  public interface SearchCallback {
    void onSearchResult(Friend[] results);
  }

  public static native FriendsPayload nativeGetLists();
  public static native void nativeRefresh();
  public static native void nativeSignup(String username);
  public static native void nativeChangeUsername(String username);
  public static native void nativeSearchByUsername(String query, SearchCallback callback);
  public static native void nativeSendRequest(String userId);
  public static native void nativeAcceptRequest(String userId);
  public static native void nativeCancelRequest(String userId);

  private static native void nativeSubscribe();
  private static native void nativeUnsubscribe();
}
