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

import com.access_company.hbbtv.HbbTvAppMgr;

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
            boolean shouldUseDefaultUA = true;
            for (String uadescfile : uadescfiles) {
                Log.i(TAG, "Obtaining the UA string from " + uadescfile);
                try {
                    java.io.FileReader fr = new java.io.FileReader(uadescfile);
                    java.io.BufferedReader br = new java.io.BufferedReader(fr);
                    String ua = br.readLine();
                    if (!TextUtils.isEmpty(ua)) {
                        Log.i(TAG, "The UA string to use is " + ua);
                        cl.appendSwitchWithValue("user-agent", ua);
                        shouldUseDefaultUA = false;
                        break;
                    }
                } catch (Exception e) {
                }
            }
            if (shouldUseDefaultUA) {
                cl.appendSwitchWithValue("user-agent", "HbbTV/1.1.1 (; CUS:; MB70; 1.0; 1.0;) LOH; Opera; CE-HTML; CE-HTML/1.0 NETRANGEMMH NetFront");
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

        /* OnAITUpdate test code start */
        if (shellUrl.startsWith("aitupdate://")) {
            mShellManager.setStartupUrl("about://blank");
            shellUrl = mShellManager.getStartupUrl();
            android.os.Handler h = new android.os.Handler();
            h.postDelayed(mTestTask1, 10000);
        }
        /* OnAITUpdate test code end */

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

    /* hbbtv -------------------------------------------------------- */

    private boolean simulateAITUpdate1(boolean in_isChannelChange) {
        Shell shell = getActiveShell();
        if (shell == null) {
            return false;
        }
        HbbTvAppMgr mgr = shell.getHbbTvAppMgr();
        if (mgr == null) {
            return false;
        }

        HbbTvAppMgr.DVBApplicationInformation[] ai_list =
                new HbbTvAppMgr.DVBApplicationInformation[14];

        ai_list[0] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[0].fApplicationID = 0;
        ai_list[0].fOrganisationID = 0;
        ai_list[0].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[0].fPriority = 0;
        ai_list[0].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_AUTOSTART;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "http://itv.mit-xperts.com/hbbtvtest/";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_HTTP;
            ai_list[0].fTransportProtocols = tp_list;
        }
        ai_list[0].fIsServiceBound = false;
        ai_list[0].fLocation = "";
        ai_list[0].fBoundaries = null;
        ai_list[0].fNames = null;
        ai_list[0].fIconLocator = null;
        ai_list[0].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_GENERIC;

        ai_list[1] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[1].fApplicationID = 0x1f5;
        ai_list[1].fOrganisationID = 19;
        ai_list[1].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[1].fPriority = 0;
        ai_list[1].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_PRESENT;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "http://itv.mit-xperts.com/";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_HTTP;
            ai_list[1].fTransportProtocols = tp_list;
        }
        ai_list[1].fIsServiceBound = false;
        ai_list[1].fLocation = "hbbtvtest/appmanager/otherapp.php?param1=value1";
        ai_list[1].fBoundaries = null;
        ai_list[1].fNames = null;
        ai_list[1].fIconLocator = null;
        ai_list[1].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_TELETEXT;

        ai_list[2] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[2].fApplicationID = 0x3;
        ai_list[2].fOrganisationID = 19;
        ai_list[2].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[2].fPriority = 0;
        ai_list[2].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_PRESENT;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "http://itv.ard.de/";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_HTTP;
            ai_list[2].fTransportProtocols = tp_list;
        }
        ai_list[2].fIsServiceBound = false;
        ai_list[2].fLocation = "hbbtv-ard/mediathek/";
        ai_list[2].fBoundaries = null;
        ai_list[2].fNames = null;
        ai_list[2].fIconLocator = null;
        ai_list[2].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_TELETEXT;

        ai_list[3] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[3].fApplicationID = 0xa;
        ai_list[3].fOrganisationID = 19;
        ai_list[3].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[3].fPriority = 0;
        ai_list[3].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_PRESENT;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "http://itv.mit-xperts.com/";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_HTTP;
            ai_list[3].fTransportProtocols = tp_list;
        }
        ai_list[3].fIsServiceBound = false;
        ai_list[3].fLocation = "hbbtvtest/";
        ai_list[3].fBoundaries = null;
        ai_list[3].fNames = null;
        ai_list[3].fIconLocator = null;
        ai_list[3].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_TELETEXT;

        ai_list[4] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[4].fApplicationID = 0x2;
        ai_list[4].fOrganisationID = 19;
        ai_list[4].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[4].fPriority = 0;
        ai_list[4].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_PRESENT;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "http://itv.ard.de/";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_HTTP;
            ai_list[4].fTransportProtocols = tp_list;
        }
        ai_list[4].fIsServiceBound = false;
        ai_list[4].fLocation = "ardepg/";
        ai_list[4].fBoundaries = null;
        ai_list[4].fNames = null;
        ai_list[4].fIconLocator = null;
        ai_list[4].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_TELETEXT;

        ai_list[5] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[5].fApplicationID = 0x14;
        ai_list[5].fOrganisationID = 19;
        ai_list[5].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[5].fPriority = 0;
        ai_list[5].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_PRESENT;;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "http://hbbtv.daserste.de/";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_HTTP;
            ai_list[5].fTransportProtocols = tp_list;
        }
        ai_list[5].fIsServiceBound = false;
        ai_list[5].fLocation = "";
        ai_list[5].fBoundaries = null;
        ai_list[5].fNames = null;
        ai_list[5].fIconLocator = null;
        ai_list[5].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_TELETEXT;

        ai_list[6] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[6].fApplicationID = 0x1;
        ai_list[6].fOrganisationID = 0x11;
        ai_list[6].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[6].fPriority = 0;
        ai_list[6].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_PRESENT;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "http://itv.mit-xperts.com/";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_HTTP;
            ai_list[6].fTransportProtocols = tp_list;
        }
        ai_list[6].fIsServiceBound = false;
        ai_list[6].fLocation = "zdfstart/index.php";
        ai_list[6].fBoundaries = null;
        ai_list[6].fNames = null;
        ai_list[6].fIconLocator = null;
        ai_list[6].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_GENERIC;

        ai_list[7] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[7].fApplicationID = 0x2;
        ai_list[7].fOrganisationID = 17;
        ai_list[7].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[7].fPriority = 0;
        ai_list[7].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_PRESENT;;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "http://hbbtv.zdf.de/";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_HTTP;
            ai_list[7].fTransportProtocols = tp_list;
        }
        ai_list[7].fIsServiceBound = false;
        ai_list[7].fLocation = "zdfmediathek/";
        ai_list[7].fBoundaries = null;
        ai_list[7].fNames = null;
        ai_list[7].fIconLocator = null;
        ai_list[7].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_GENERIC;

        ai_list[8] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[8].fApplicationID = 6; //blue
        ai_list[8].fOrganisationID = 19;
        ai_list[8].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[8].fPriority = 0;
        ai_list[8].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_PRESENT;;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "http://itv.mit-xperts.com/";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_HTTP;
            ai_list[8].fTransportProtocols = tp_list;
        }
        ai_list[8].fIsServiceBound = false;
        ai_list[8].fLocation = "zdfstart/index.php";
        ai_list[8].fBoundaries = null;
        ai_list[8].fNames = null;
        ai_list[8].fIconLocator = null;
        ai_list[8].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_GENERIC;

        ai_list[9] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[9].fApplicationID = 1; //0
        ai_list[9].fOrganisationID = 19;
        ai_list[9].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[9].fPriority = 0;
        ai_list[9].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_PRESENT;;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "http://itv.ard.de/";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_HTTP;
            ai_list[9].fTransportProtocols = tp_list;
        }
        ai_list[9].fIsServiceBound = false;
        ai_list[9].fLocation = "ardstart/index.php";
        ai_list[9].fBoundaries = null;
        ai_list[9].fNames = null;
        ai_list[9].fIconLocator = null;
        ai_list[9].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_GENERIC;

        ai_list[10] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[10].fApplicationID = 17;//yellow
        ai_list[10].fOrganisationID = 19;
        ai_list[10].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[10].fPriority = 0;
        ai_list[10].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_PRESENT;;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "http://hbbtv.ardmediathek.de/";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_HTTP;
            ai_list[10].fTransportProtocols = tp_list;
        }
        ai_list[10].fIsServiceBound = false;
        ai_list[10].fLocation = "hbbtv-ard/mediathek/";
        ai_list[10].fBoundaries = null;
        ai_list[10].fNames = null;
        ai_list[10].fIconLocator = null;
        ai_list[10].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_TELETEXT;

        ai_list[11] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[11].fApplicationID = 4; // blue
        ai_list[11].fOrganisationID = 19;
        ai_list[11].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[11].fPriority = 0;
        ai_list[11].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_PRESENT;;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "dvb://current.ait/13.1f7";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_DSMCC;
            ai_list[11].fTransportProtocols = tp_list;
        }
        ai_list[11].fIsServiceBound = false;
        ai_list[11].fLocation = "ardtext";
        ai_list[11].fBoundaries = null;
        ai_list[11].fNames = null;
        ai_list[11].fIconLocator = null;
        ai_list[11].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_GENERIC;

        ai_list[12] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[12].fApplicationID = 3; //heute journal plus
        ai_list[12].fOrganisationID = 17;
        ai_list[12].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[12].fPriority = 0;
        ai_list[12].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_PRESENT;;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "http://hbbtv.zdf.de/";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_HTTP;
            ai_list[12].fTransportProtocols = tp_list;
        }
        ai_list[12].fIsServiceBound = false;
        ai_list[12].fLocation = "zdfhjplus";
        ai_list[12].fBoundaries = null;
        ai_list[12].fNames = null;
        ai_list[12].fIconLocator = null;
        ai_list[12].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_GENERIC;

        ai_list[13] = new HbbTvAppMgr.DVBApplicationInformation();
        ai_list[13].fApplicationID = 0x1f7; // test 8 starting from DSMCC
        ai_list[13].fOrganisationID = 19;
        ai_list[13].fVisibility = HbbTvAppMgr.DVBApplicationVisibility.DVB_NOT_VISIBLE_ALL;
        ai_list[13].fPriority = 0;
        ai_list[13].fControlCode = HbbTvAppMgr.DVBApplicationControlCode.DVBAPPLICATION_CONTROLCODE_PRESENT;;
        {
            HbbTvAppMgr.DVBApplicationTransportProtocol[] tp_list =
                    new HbbTvAppMgr.DVBApplicationTransportProtocol[1];
            tp_list[0] = new HbbTvAppMgr.DVBApplicationTransportProtocol();
            tp_list[0].fBaseURL = "dvb://current.ait/13.1f7";
            tp_list[0].fProtocolID = HbbTvAppMgr.DVBApplicationProtocol.DVBAPPLICATION_PROTOCOL_DSMCC;
            ai_list[13].fTransportProtocols = tp_list;
        }
        ai_list[13].fIsServiceBound = false;
        ai_list[13].fLocation = "";
        ai_list[13].fBoundaries = null;
        ai_list[13].fNames = null;
        ai_list[13].fIconLocator = null;
        ai_list[13].fUsageType = HbbTvAppMgr.OIPFApplicationUsageType.OIPFAPPLICATION_USAGE_TYPE_GENERIC;

        HbbTvAppMgr.OnAITUpdateReturnValue rval =
                mgr.OnAITUpdate(ai_list, in_isChannelChange, false);

        Log.i(TAG, "rval == " + rval);

        if (rval == HbbTvAppMgr.OnAITUpdateReturnValue.E_OK) {
            return true;
        }
        return false;
    }

    private Runnable mTestTask1 = new Runnable() {
        public void run() {
            Log.i(TAG, "mTestTask1");
            boolean bv = simulateAITUpdate1(true);
            if (bv) {
                android.os.Handler h = new android.os.Handler();
                h.postDelayed(mTestTask2, 10000);
            }
        }
    };
    private Runnable mTestTask2 = new Runnable() {
        public void run() {
            Log.i(TAG, "mTestTask2");
            simulateAITUpdate1(false);
        }
    };
}
