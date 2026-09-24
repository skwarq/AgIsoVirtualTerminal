package com.openagriculture.agisovirtualterminal;

import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.content.res.Configuration;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.rmsl.juce.JuceActivity;

/** JUCE activity configured for immersive, fixed-landscape VT display. */
public final class MainActivity extends JuceActivity {
    private int lastImeInset = -1;
    private PowerManager.WakeLock screenWakeLock;

    private static native void onImeInsetsChanged(int bottomInsetPixels);

    private void reportImeInset(int bottomInsetPixels) {
        if (bottomInsetPixels == lastImeInset)
            return;
        lastImeInset = bottomInsetPixels;
        onImeInsetsChanged(bottomInsetPixels);
    }

    private void installImeInsetsListener() {
        final View decorView = getWindow().getDecorView();
        ViewCompat.setOnApplyWindowInsetsListener(decorView, (view, insets) -> {
            final int imeType = WindowInsetsCompat.Type.ime();
            final int imeBottom = insets.isVisible(imeType) ? insets.getInsets(imeType).bottom : 0;
            reportImeInset(imeBottom);
            return insets;
        });
        ViewCompat.requestApplyInsets(decorView);
    }

    /** Re-deliver the current IME state when a dialog subscribes after the keyboard opened. */
    public void requestCurrentImeInsets() {
        runOnUiThread(() -> {
            lastImeInset = -1;
            ViewCompat.requestApplyInsets(getWindow().getDecorView());
        });
    }

    private void keepScreenAwake() {
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getDecorView().setKeepScreenOn(true);
    }

    @SuppressWarnings("deprecation")
    private void acquireScreenWakeLock() {
        if (screenWakeLock == null) {
            final PowerManager powerManager = (PowerManager) getSystemService(POWER_SERVICE);
            if (powerManager != null)
                screenWakeLock = powerManager.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK,
                        "AgIsoVirtualTerminal:KeepScreenOn");
        }
        if (screenWakeLock != null && !screenWakeLock.isHeld())
            screenWakeLock.acquire();
    }

    private void releaseScreenWakeLock() {
        if (screenWakeLock != null && screenWakeLock.isHeld())
            screenWakeLock.release();
    }

    private void enterImmersiveFullscreen() {
        final View decorView = getWindow().getDecorView();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            getWindow().setDecorFitsSystemWindows(false);
            final WindowInsetsController controller = decorView.getWindowInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
                controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        keepScreenAwake();
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        installImeInsetsListener();
        getWindow().getDecorView().post(this::enterImmersiveFullscreen);
    }

    @Override
    protected void onResume() {
        super.onResume();
        keepScreenAwake();
        acquireScreenWakeLock();
        enterImmersiveFullscreen();
    }

    @Override
    protected void onStop() {
        releaseScreenWakeLock();
        super.onStop();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            keepScreenAwake();
            enterImmersiveFullscreen();
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        getWindow().getDecorView().post(() -> {
            keepScreenAwake();
            getWindow().getDecorView().requestLayout();
            getWindow().getDecorView().requestApplyInsets();
            enterImmersiveFullscreen();
        });
    }
}
