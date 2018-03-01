// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.access_company.nfbe.content_shell_apk;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Toast;

import org.chromium.base.BaseSwitches;
import org.chromium.base.CommandLine;
import org.chromium.base.MemoryPressureListener;
import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.content.app.ContentApplication;
import org.chromium.content.browser.BrowserStartupController;
import org.chromium.content.browser.ContentViewCore;
import org.chromium.content.browser.DeviceUtils;
import org.chromium.content.common.ContentSwitches;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_shell.Shell;
import org.chromium.content_shell.ShellManager;
import org.chromium.ui.base.ActivityWindowAndroid;

/**
 * Activity for managing the Content Shell.
 */
public class ContentShellActivity extends Activity {

    private static final String TAG = "ContentShellActivity";

    private static final String ACTIVE_SHELL_URL_KEY = "activeUrl";
    public static final String COMMAND_LINE_ARGS_KEY = "commandLineArgs";

    private ShellManager mShellManager;
    private ActivityWindowAndroid mWindowAndroid;
    private Intent mLastSentIntent;

    @Override
    @SuppressFBWarnings("DM_EXIT")
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Temporally change default visibility to true for demo on Android STB.
        //boolean shellToolBarVisibilityDefault = false;
        boolean shellToolBarVisibilityDefault = true;

        // Initializing the command line must occur before loading the library.
        if (!CommandLine.isInitialized()) {
            ContentApplication.initCommandLine(this);
            String[] commandLineParams = getCommandLineParamsFromIntent(getIntent());
            if (commandLineParams != null) {
                CommandLine.getInstance().appendSwitchesAndArguments(commandLineParams);
            }
        }
        CommandLine cl = CommandLine.getInstance();
        {
            // read a custom UA from file.
            String extdir = android.os.Environment.getExternalStorageDirectory().getPath();
            String[] uadescfiles = {
                extdir + "/nfbe/config_ua.txt",
                "/mnt/internal_sd/nfbe/config_ua.txt",
                mntSdCard() + "/nfbe/config_ua.txt",
            };
            for (String uadescfile : uadescfiles) {
                Log.i(TAG, "Obtaining the UA string from " + uadescfile);
                try {
                    java.io.FileReader fr = new java.io.FileReader(uadescfile);
                    java.io.BufferedReader br = new java.io.BufferedReader(fr);
                    String ua = br.readLine();
                    if (!TextUtils.isEmpty(ua)) {
                        Log.i(TAG, "The UA string to use is " + ua);
                        cl.appendSwitchWithValue("user-agent", ua);
                        break;
                    }
                } catch (Exception e) {
                }
            }
        }
        {
            boolean disableGestureRequirementForMediaPlayback = true;
            String acsEnableAutomaticScaling = null;
            String extdir = android.os.Environment.getExternalStorageDirectory().getPath();
            String[] configfiles = {
                extdir + "/nfbe/config.txt",
                "/mnt/internal_sd/nfbe/config.txt",
                mntSdCard() + "/nfbe/config.txt",
            };
            for (String configfile : configfiles) {
                java.io.FileReader fr = null;
                java.io.BufferedReader br = null;
                try {
                    fr = new java.io.FileReader(configfile);
                    br = new java.io.BufferedReader(fr);
                } catch (Exception e) {
                    if (fr != null) {
                        try {
                            fr.close();
                        } catch (Exception e1) {
                        }
                    }
                    continue;
                }
                do {
                    String line = null;
                    try {
                        line = br.readLine();
                    } catch (Exception e) {
                        break;
                    }
                    if (line == null) {
                        break;
                    }
                    line = line.replaceAll("\\s","");
                    if (0 < line.length()) {
                        String[] keyValue = line.split("=");
                        if (keyValue.length == 2) {
                            if ("toolbar.visibility.default".equals(keyValue[0])) {
                                if ("true".equals(keyValue[1])) {
                                    shellToolBarVisibilityDefault = true;
                                } else if ("false".equals(keyValue[1])) {
                                    shellToolBarVisibilityDefault = false;
                                }
                            }
                            if ("user_gesture_required_for_media_playback".equals(keyValue[0])) {
                                if ("true".equals(keyValue[1])) {
                                    disableGestureRequirementForMediaPlayback = false;
                                } else if ("false".equals(keyValue[1])) {
                                    disableGestureRequirementForMediaPlayback = true;
                                }
                            }
                            if ("acs_enable_automatic_scaling".equals(keyValue[0])) {
                                if ("enable".equals(keyValue[1]) || "disable".equals(keyValue[1])) {
                                    acsEnableAutomaticScaling = keyValue[1];
                                }
                            }
                        }
                    }
                } while (true);
                try {
                    fr.close();
                } catch (Exception e) {
                }
                break;
            }
            // if OS_ANDROID, the default value of |user_gesture_required_for_media_playback|
            // is true, so we need to add this option to make that value to false.
            if (disableGestureRequirementForMediaPlayback) {
                cl.appendSwitch("disable-gesture-requirement-for-media-playback");
            }
            if (acsEnableAutomaticScaling != null) {
                cl.appendSwitchWithValue("acs-enable-automatic-scaling",
                        acsEnableAutomaticScaling);
            }
        }
        waitForDebuggerIfNeeded();

        DeviceUtils.addDeviceSpecificUserAgentSwitch(this);
        try {
            LibraryLoader.get(LibraryProcessType.PROCESS_BROWSER)
                    .ensureInitialized(getApplicationContext());
        } catch (ProcessInitException e) {
            Log.e(TAG, "ContentView initialization failed.", e);
            // Since the library failed to initialize nothing in the application
            // can work, so kill the whole application not just the activity
            System.exit(-1);
            return;
        }

        setContentView(R.layout.content_shell_activity);

        enterFullscreen();
        {
            View decorView = getWindow().getDecorView();
            decorView.setOnSystemUiVisibilityChangeListener
                    (new View.OnSystemUiVisibilityChangeListener() {
                @Override
                public void onSystemUiVisibilityChange(int visibility) {
                    if (visibility == android.view.View.SYSTEM_UI_FLAG_VISIBLE) {
                        Shell activeView = getActiveShell();
                        if (activeView != null) {
                            activeView.setToolBarVisibility(true);
                        }
                    }
                }
            });
        }

        mShellManager = (ShellManager) findViewById(R.id.shell_container);
        final boolean listenToActivityState = true;
        mWindowAndroid = new ActivityWindowAndroid(this, listenToActivityState);
        mWindowAndroid.restoreInstanceState(savedInstanceState);
        mShellManager.setWindow(mWindowAndroid);
        // Set up the animation placeholder to be the SurfaceView. This disables the
        // SurfaceView's 'hole' clipping during animations that are notified to the window.
        mWindowAndroid.setAnimationPlaceholderView(
                mShellManager.getContentViewRenderView().getSurfaceView());

        mShellManager.setRequestFullscreenShell(new Shell.RequestFullscreenShell() {
            @Override
            public void requestFullscreen() {
                enterFullscreen();
            }
        });
        mShellManager.setShellToolBarVisibilityDefault(shellToolBarVisibilityDefault);
	mShellManager.setStartupUrl("about:blank");
	{
            // read the start-up URL from a file
            String extdir = android.os.Environment.getExternalStorageDirectory().getPath();
            String[] urldescfiles = {
                extdir + "/nfbe/config_url.txt",
                "/mnt/internal_sd/nfbe/config_url.txt",
                mntSdCard() + "/nfbe/config_url.txt",
                "/mnt/internal_sd/url.txt",
                mntSdCard() + "/url.txt",
            };
            for (String urldescfile : urldescfiles) {
                Log.i(TAG, "Obtaining the startup URL from " + urldescfile);
                try {
                    java.io.FileReader fr = new java.io.FileReader(urldescfile);
                    java.io.BufferedReader br = new java.io.BufferedReader(fr);
                    String url = br.readLine();
                    if (!TextUtils.isEmpty(url)) {
                        Log.i(TAG, "The startup URL is " + url);
                        mShellManager.setStartupUrl(url);
                        break;
                    }
                } catch (Exception e) {
                }
            }
        }

        String startupUrl = getUrlFromIntent(getIntent());
        if (!TextUtils.isEmpty(startupUrl)) {
            mShellManager.setStartupUrl(Shell.sanitizeUrl(startupUrl));
        }

        if (CommandLine.getInstance().hasSwitch(ContentSwitches.RUN_LAYOUT_TEST)) {
            try {
                BrowserStartupController.get(this, LibraryProcessType.PROCESS_BROWSER)
                        .startBrowserProcessesSync(false);
            } catch (ProcessInitException e) {
                Log.e(TAG, "Failed to load native library.", e);
                System.exit(-1);
            }
        } else {
            try {
                BrowserStartupController.get(this, LibraryProcessType.PROCESS_BROWSER)
                        .startBrowserProcessesAsync(
                                new BrowserStartupController.StartupCallback() {
                                    @Override
                                    public void onSuccess(boolean alreadyStarted) {
                                        finishInitialization(savedInstanceState);
                                    }

                                    @Override
                                    public void onFailure() {
                                        initializationFailed();
                                    }
                                });
            } catch (ProcessInitException e) {
                Log.e(TAG, "Unable to load native library.", e);
                System.exit(-1);
            }
        }
    }

    private void finishInitialization(Bundle savedInstanceState) {
        String shellUrl = mShellManager.getStartupUrl();
        if (savedInstanceState != null
                && savedInstanceState.containsKey(ACTIVE_SHELL_URL_KEY)) {
            shellUrl = savedInstanceState.getString(ACTIVE_SHELL_URL_KEY);
        }
        mShellManager.launchShell(shellUrl);
    }

    private void initializationFailed() {
        Log.e(TAG, "ContentView initialization failed.");
        Toast.makeText(ContentShellActivity.this,
                R.string.browser_process_initialization_failed,
                Toast.LENGTH_SHORT).show();
        finish();
    }

    private void enterFullscreen() {
        int SDK_INT = android.os.Build.VERSION.SDK_INT;
        int newVis = View.SYSTEM_UI_FLAG_VISIBLE;
        if (11 <= SDK_INT && SDK_INT < 14) {
            newVis = View.STATUS_BAR_HIDDEN;
        } else if (14 <= SDK_INT && SDK_INT < 19) {
            newVis = (View.SYSTEM_UI_FLAG_FULLSCREEN
                      | View.SYSTEM_UI_FLAG_LOW_PROFILE);
        } else if (19 <= SDK_INT) {
            newVis = (View.SYSTEM_UI_FLAG_FULLSCREEN
                      | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                      | View.SYSTEM_UI_FLAG_IMMERSIVE);
        }
        getWindow().getDecorView().setSystemUiVisibility(newVis);
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        ContentViewCore contentViewCore = getActiveContentViewCore();
        if (contentViewCore != null) {
            outState.putString(ACTIVE_SHELL_URL_KEY, contentViewCore.getWebContents().getUrl());
        }

        mWindowAndroid.saveInstanceState(outState);
    }

    private void waitForDebuggerIfNeeded() {
        if (CommandLine.getInstance().hasSwitch(BaseSwitches.WAIT_FOR_JAVA_DEBUGGER)) {
            Log.e(TAG, "Waiting for Java debugger to connect...");
            android.os.Debug.waitForDebugger();
            Log.e(TAG, "Java debugger connected. Resuming execution.");
        }
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            Shell activeView = getActiveShell();
            if (activeView.exitFullscreenModeIfEnabled()) {
                return true;
            }
            ContentViewCore contentViewCore = getActiveContentViewCore();
            if (contentViewCore != null && contentViewCore.getWebContents()
                    .getNavigationController().canGoBack()) {
                contentViewCore.getWebContents().getNavigationController().goBack();
                return true;
            }
        }

        Log.i(TAG, "Received key event with keycode " + keyCode);
        if (keyCode == KeyEvent.KEYCODE_MENU) {
            Log.i(TAG, "Focusing the toolbar");
            mShellManager.focusToolbar();
        }

        return super.onKeyUp(keyCode, event);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        if (getCommandLineParamsFromIntent(intent) != null) {
            Log.i(TAG, "Ignoring command line params: can only be set when creating the activity.");
        }

        if (MemoryPressureListener.handleDebugIntent(this, intent.getAction())) return;

        String url = getUrlFromIntent(intent);
        if (!TextUtils.isEmpty(url)) {
            Shell activeView = getActiveShell();
            if (activeView != null) {
                activeView.loadUrl(url);
            }
        }
    }

    @Override
    protected void onStart() {
        super.onStart();

        ContentViewCore contentViewCore = getActiveContentViewCore();
        if (contentViewCore != null) contentViewCore.onShow();
    }

    @Override
    protected void onResume() {
        super.onResume();
        enterFullscreen();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        mWindowAndroid.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void startActivity(Intent i) {
        mLastSentIntent = i;
        super.startActivity(i);
    }

    @Override
    protected void onDestroy() {
        if (mShellManager != null) mShellManager.destroy();
        super.onDestroy();
    }

    public Intent getLastSentIntent() {
        return mLastSentIntent;
    }

    private static String getUrlFromIntent(Intent intent) {
        return intent != null ? intent.getDataString() : null;
    }

    private static String[] getCommandLineParamsFromIntent(Intent intent) {
        return intent != null ? intent.getStringArrayExtra(COMMAND_LINE_ARGS_KEY) : null;
    }

    /**
     * @return The {@link ShellManager} configured for the activity or null if it has not been
     *         created yet.
     */
    public ShellManager getShellManager() {
        return mShellManager;
    }

    /**
     * @return The currently visible {@link Shell} or null if one is not showing.
     */
    public Shell getActiveShell() {
        return mShellManager != null ? mShellManager.getActiveShell() : null;
    }

    /**
     * @return The {@link ContentViewCore} owned by the currently visible {@link Shell} or null if
     *         one is not showing.
     */
    public ContentViewCore getActiveContentViewCore() {
        Shell shell = getActiveShell();
        return shell != null ? shell.getContentViewCore() : null;
    }

    /**
     * @return The {@link WebContents} owned by the currently visible {@link Shell} or null if
     *         one is not showing.
     */
    public WebContents getActiveWebContents() {
        Shell shell = getActiveShell();
        return shell != null ? shell.getWebContents() : null;
    }

    /**
     * @return The path where the sdcard is mounted on the system. This is a kludge needed
     *         for lint to properly parse the file.
     */
    public String mntSdCard() {
        return "/mnt/sdcard";
    }
}
