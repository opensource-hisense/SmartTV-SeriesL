// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_shell;

import android.app.Activity;
import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.SurfaceView;
import android.view.View;
import android.util.Log;
import android.widget.FrameLayout;

import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.browser.ActivityContentVideoViewEmbedder;
import org.chromium.content.browser.ContentVideoViewEmbedder;
import org.chromium.content.browser.ContentViewClient;
import org.chromium.content.browser.ContentViewCore;
import org.chromium.content.browser.ContentViewRenderView;
import org.chromium.ui.base.WindowAndroid;

import java.util.ArrayList;

/**
 * Container and generator of ShellViews.
 */
@JNINamespace("content")
public class ShellManager extends FrameLayout {

    public static final String DEFAULT_SHELL_URL = "http://www.google.com";
    private static boolean sStartup = true;
    private WindowAndroid mWindow;
    private Shell mActiveShell;

    private ArrayList<Shell> mInactiveShells;

    private boolean mShellToolBarVisibilityDefault = true;

    private String mStartupUrl = DEFAULT_SHELL_URL;

    // The target for all content rendering.
    private ContentViewRenderView mContentViewRenderView;
    private ContentViewClient mContentViewClient;

    private SurfaceView mDummyBroadcastVideoSurfaceView;
    /**
     * Constructor for inflating via XML.
     */
    public ShellManager(final Context context, AttributeSet attrs) {
        super(context, attrs);
        mInactiveShells =  new ArrayList<Shell>();
        nativeInit(this);
        // This view shows dummy video for video/broadcast object.
        // The current implementation shows just green.
        mDummyBroadcastVideoSurfaceView = new SurfaceView(getContext());
        mDummyBroadcastVideoSurfaceView.setBackgroundColor(0xff008000);

        mContentViewClient = new ContentViewClient() {
            @Override
            public ContentVideoViewEmbedder getContentVideoViewEmbedder() {
                return new ActivityContentVideoViewEmbedder((Activity) context) {
                    @Override
                    public void enterFullscreenVideo(View view) {
                        super.enterFullscreenVideo(view);
                        setOverlayVideoMode(true);
                        mDummyBroadcastVideoSurfaceView.setVisibility(View.GONE);
                    }

                    @Override
                    public void exitFullscreenVideo() {
                        super.exitFullscreenVideo();
                        mDummyBroadcastVideoSurfaceView.setVisibility(View.VISIBLE);
                        setOverlayVideoMode(false);
                    }
                };
            }
        };
    }

    /**
     * @param window The window used to generate all shells.
     */
    public void setWindow(WindowAndroid window) {
        setWindow(window, true);
    }

    /**
     * @param window The window used to generate all shells.
     * @param initialLoadingNeeded Whether initial loading is needed or not.
     */
    @VisibleForTesting
    public void setWindow(WindowAndroid window, final boolean initialLoadingNeeded) {
        assert window != null;
        mWindow = window;
        mContentViewRenderView = new ContentViewRenderView(getContext()) {
            @Override
            protected void onReadyToRender() {
                if (sStartup) {
                    Log.i("ShellManager", "mActiveShell.loadUrl " + mStartupUrl);
                    if (initialLoadingNeeded) mActiveShell.loadUrl(mStartupUrl);
                    sStartup = false;
                }
            }
        };
        mContentViewRenderView.addView(mDummyBroadcastVideoSurfaceView);
        mContentViewRenderView.onNativeLibraryLoaded(window);
    }

    /**
     * @return The window used to generate all shells.
     */
    public WindowAndroid getWindow() {
        return mWindow;
    }

    /**
     * Get the ContentViewRenderView.
     */
    public ContentViewRenderView getContentViewRenderView() {
        return mContentViewRenderView;
    }

    /**
     * Sets the startup URL for new shell windows.
     */
    public void setStartupUrl(String url) {
        Log.i("ShellManager", "setStartupUrl " + url);
        mStartupUrl = url;
    }

    /**
     * @return The startup URL for new shell windows.
     */
    public String getStartupUrl() {
        return mStartupUrl;
    }

    /**
     * @return The currently visible shell view or null if one is not showing.
     */
    public Shell getActiveShell() {
        return mActiveShell;
    }

    public void setShellToolBarVisibilityDefault(boolean visibility) {
        mShellToolBarVisibilityDefault = visibility;
    }

     /**
      * Focuses the toolbar in the active shell.  This allows to input an URL on platforms where only a remote is available.
      */
     public void focusToolbar() {
        if (mActiveShell != null) {
            mActiveShell.focusToolbar();
        }
     }

    /**
     * Creates a new shell pointing to the specified URL.
     * @param url The URL the shell should load upon creation.
     */
    public void launchShell(String url) {
        ThreadUtils.assertOnUiThread();
        Shell previousShell = mActiveShell;
        nativeLaunchShell(url);
        if (previousShell != null) previousShell.close();
    }

    /**
     * Enter or leave overlay video mode.
     * @param enabled Whether overlay mode is enabled.
     */
    public void setOverlayVideoMode(boolean enabled) {
        if (mContentViewRenderView == null) return;
        mContentViewRenderView.setOverlayVideoMode(enabled);
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private Object createShell(long nativeShellPtr) {
        assert mContentViewRenderView != null;
        LayoutInflater inflater =
                (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        Shell shellView = (Shell) inflater.inflate(R.layout.shell_view, null);
        shellView.initialize(nativeShellPtr, mWindow, mContentViewClient);
        shellView.setToolBarVisibility(mShellToolBarVisibilityDefault);
        shellView.setRequestFullscreenShell(mRequestFullscreenShell);

        if (mActiveShell != null) {
            hideShell(mActiveShell);
            mInactiveShells.add(mActiveShell);
            mActiveShell = null;
        }

        showShell(shellView);
        return shellView;
    }

    private void showShell(Shell shellView) {
        shellView.setContentViewRenderView(mContentViewRenderView);
        addView(shellView, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
        mActiveShell = shellView;
        ContentViewCore contentViewCore = mActiveShell.getContentViewCore();
        if (contentViewCore != null) {
            mContentViewRenderView.setCurrentContentViewCore(contentViewCore);
            contentViewCore.onShow();
        }
    }

    @CalledByNative
    private void removeShell(Shell shellView) {
        if (shellView == mActiveShell) mActiveShell = null;
        hideShell(shellView);
        mInactiveShells.remove(shellView);
        if (mActiveShell == null) {
            int n = mInactiveShells.size();
            if (0 < n) {
                Shell newActiveShell = mInactiveShells.remove(n - 1);
                showShell(newActiveShell);
            }
        }
    }

    private void hideShell(Shell shellView) {
        if (shellView.getParent() == null) return;
        ContentViewCore contentViewCore = shellView.getContentViewCore();
        if (contentViewCore != null) contentViewCore.onHide();
        shellView.setContentViewRenderView(null);
        removeView(shellView);
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void focus(long nativeShellPtr) {
        Shell inactiveShell = null;
        for (Shell shell : mInactiveShells) {
            if (shell.getNativeShell() == nativeShellPtr) {
                inactiveShell = shell;
            }
        }
        if (inactiveShell == null) {
            //Log.e("chromium", "Called setNextShell() with wrong nativeShellPtr: " + Long.toHexString(nativeShellPtr));
            return;
        }
        mInactiveShells.remove(inactiveShell);
        if (mActiveShell != null) {
            hideShell(mActiveShell);
            mInactiveShells.add(mActiveShell);
            mActiveShell = null;
        }

        showShell(inactiveShell);
    }

    /**
     * Destroys the Shell manager and associated components.
     */
    public void destroy() {
        // Remove active shell (Currently single shell support only available).
        removeShell(mActiveShell);
        mContentViewRenderView.destroy();
        mContentViewRenderView = null;
    }

    private static native void nativeInit(Object shellManagerInstance);
    private static native void nativeLaunchShell(String url);

    Shell.RequestFullscreenShell mRequestFullscreenShell;
    public void setRequestFullscreenShell(Shell.RequestFullscreenShell bo) {
        mRequestFullscreenShell = bo;
    }
}
