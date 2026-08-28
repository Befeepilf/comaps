package app.organicmaps.settings;
import androidx.annotation.Keep;

import android.os.Bundle;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import app.organicmaps.R;
import app.organicmaps.util.WindowInsetUtils.ScrollableContentInsetsListener;

@Keep
public class ExplorerProInfoFragment extends BaseSettingsFragment
{
  @Override
  protected int getLayoutRes()
  {
    return R.layout.fragment_explorer_pro_info;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    ViewCompat.setOnApplyWindowInsetsListener(view, new ScrollableContentInsetsListener(view));
  }
}
