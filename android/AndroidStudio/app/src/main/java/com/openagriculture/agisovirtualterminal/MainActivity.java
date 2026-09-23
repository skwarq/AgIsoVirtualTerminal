package com.openagriculture.agisovirtualterminal;

import android.os.Build;
import android.os.Bundle;
import android.content.res.Configuration;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.graphics.Rect;

import com.rmsl.juce.JuceActivity;

/** JUCE activity configured for landscape VT display and IME rotation handling. */
public final class MainActivity extends JuceActivity {
    private boolean keyboardOrientation = false;

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

    private void installImeResizeHandling() {
        final View decorView = getWindow().getDecorView();
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            decorView.getViewTreeObserver().addOnGlobalLayoutListener(() -> {
                final Rect visibleFrame = new Rect();
                decorView.getWindowVisibleDisplayFrame(visibleFrame);
                final int obscuredHeight = decorView.getRootView().getHeight() - visibleFrame.bottom;
                final boolean imeVisible = obscuredHeight > decorView.getRootView().getHeight() / 5;
                updateKeyboardOrientation(imeVisible);
                if (!imeVisible)
                    enterImmersiveFullscreen();
            });
            return;
        }

        decorView.setOnApplyWindowInsetsListener((view, insets) -> {
            if (insets.isVisible(WindowInsets.Type.ime())) {
                updateKeyboardOrientation(true);
            } else {
                updateKeyboardOrientation(false);
                enterImmersiveFullscreen();
            }
            return insets;
        });
        decorView.requestApplyInsets();
    }

    private void updateKeyboardOrientation(boolean keyboardVisible) {
        if (keyboardVisible == keyboardOrientation)
            return;
        keyboardOrientation = keyboardVisible;
        setRequestedOrientation(keyboardVisible
                ? android.content.pm.ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
                : android.content.pm.ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        installImeResizeHandling();
        getWindow().getDecorView().post(this::enterImmersiveFullscreen);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (!keyboardOrientation)
            enterImmersiveFullscreen();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
            if (!keyboardOrientation)
                enterImmersiveFullscreen();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        getWindow().getDecorView().post(() -> {
            getWindow().getDecorView().requestLayout();
            if (!keyboardOrientation)
                enterImmersiveFullscreen();
        });
    }
}
