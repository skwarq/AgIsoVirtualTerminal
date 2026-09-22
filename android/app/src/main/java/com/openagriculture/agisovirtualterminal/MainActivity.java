package com.openagriculture.agisovirtualterminal;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

/** Initial Android entry point for the TCP-backed virtual terminal. */
public final class MainActivity extends Activity {
    static {
        System.loadLibrary("agisovirtualterminal_native");
    }

    private static native String nativeBuildStatus();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        TextView status = new TextView(this);
        status.setText("AgIsoVirtualTerminal\n" + nativeBuildStatus());
        status.setPadding(32, 32, 32, 32);
        setContentView(status);
    }
}
