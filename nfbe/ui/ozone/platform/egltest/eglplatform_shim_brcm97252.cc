// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/egltest/eglplatform_shim.h"

#include <string.h>
#include <directfb.h>
#include <directfb_strings.h>
#include <direct/util.h>

#include <EGL/egl.h>

#ifdef __cplusplus
extern "C" {
#endif

const int kDefaultWidth = 1280;
const int kDefaultHeight = 720;



extern "C" {
typedef void *DBPL_PlatformHandle;
extern void DBPL_RegisterDirectFBDisplayPlatform(DBPL_PlatformHandle *handle, IDirectFB *dfb);
extern void DBPL_UnregisterDirectFBDisplayPlatform(DBPL_PlatformHandle handle);
}
static DBPL_PlatformHandle gDBPL = NULL;

static IDirectFB                *gDFB = NULL;
static IDirectFBSurface         *gDFBSurface = NULL;

// utility checking macros
#define DFBCHECK(x...)\
{\
    DFBResult err = (DFBResult)(x);\
        if (err != DFB_OK) {\
            fprintf(stderr, "[eglplatfomr_shim_brcm97252] %s <%d>:\n\t", __FILE__, __LINE__);\
                DirectFBError/*Fatal*/(#x, err);\
        }\
}


#define DFBCHECK_RET(x...)\
{\
    DFBResult err = (DFBResult)(x);\
        if (err != DFB_OK) {\
            fprintf(stderr, "[eglplatfomr_shim_brcm97252] %s <%d>:\n\t", __FILE__, __LINE__);\
                DirectFBError/*Fatal*/(#x, err);\
            return false;\
        }\
}

const char* ShimQueryString(int name) {
    fprintf(stdout, "[eglplatfomr_shim_brcm97252] %s:%d \n", __FUNCTION__, __LINE__);
  switch (name) {
    case SHIM_EGL_LIBRARY:
      return "libv3ddriver.so";
    case SHIM_GLES_LIBRARY:
      // on Broadcom platforms both EGL and GLES APIs are available
      // fromt the same library
      return "libv3ddriver.so";
    default:
      return NULL;
  }
}

bool ShimInitialize(void) {
    int argcZero = 0;
    fprintf(stdout, "[eglplatfomr_shim_brcm97252] %s:%d \n", __FUNCTION__, __LINE__);

    // the BRCOM_PLATFORM always needs the DIRECTFB initialzation to create an EGL window
    DFBCHECK_RET(DirectFBInit(&argcZero, (char ***) 0));
    fprintf(stdout, "[eglplatfomr_shim_brcm97252] DirectFB: DirectFBInit OK\n");

    DirectFBSetOption ("bg-none", NULL);
    DirectFBSetOption ("no-init-layer", NULL);

    /*
     * Create the DirectFB super interface
     */
    DFBCHECK_RET(DirectFBCreate(&gDFB));
    fprintf(stdout, "[eglplatfomr_shim_brcm97252] DirectFB: DirectFBCreate OK\n");

    /* On the embedded platforms use FULLSCREEN */
    DFBCHECK(gDFB->SetCooperativeLevel(gDFB, DFSCL_FULLSCREEN/*DFSCL_EXCLUSIVE DFSCL_NORMAL*/));

    DBPL_RegisterDirectFBDisplayPlatform(&gDBPL, gDFB);

    return true;
}

bool ShimTerminate(void) {
    fprintf(stdout, "[eglplatfomr_shim_brcm97252] %s:%d \n", __FUNCTION__, __LINE__);

    if (gDBPL) {
        DBPL_UnregisterDirectFBDisplayPlatform(gDBPL);
        gDBPL = NULL;
    }

    //gDFBEventsBuffer = NULL;
    gDFB = NULL;

    return true;
}

//FIXME: if this is called 2 times it overwrites gDFBSurface
ShimNativeWindowId ShimCreateWindow(void) {
    static uint32_t windowidx = 0;
    DFBSurfaceDescription dsc;
    fprintf(stdout, "[eglplatfomr_shim_brcm97252] %s:%d \n", __FUNCTION__, __LINE__);
    /* Get the main primary surface, i.e. the surface of the primary layer. */
    dsc.flags = (DFBSurfaceDescriptionFlags)(DSDESC_CAPS | DSDESC_PIXELFORMAT);

    /* On the embedded platforms using the video memory brings some speed sometimes */
    dsc.caps = (DFBSurfaceCapabilities)(DSCAPS_PRIMARY | DSCAPS_DOUBLE | DSCAPS_VIDEOONLY);

    // Broadcom platforms need this to render properly the color channels
    dsc.pixelformat = DSPF_ABGR;

    DFBCHECK(gDFB->CreateSurface( gDFB, &dsc, &gDFBSurface ));

    if (!gDFBSurface) {
        return 0;
    }

    windowidx++;
    fprintf(stdout, "[eglplatfomr_shim_brcm97252] DirectFB: CreateSurface OK : idx %d surface %p\n",
            windowidx, gDFBSurface);
    // currently we support only one window
    return windowidx;
}

bool ShimQueryWindow(ShimNativeWindowId window_id, int attribute, int* value) {
    fprintf(stdout, "[eglplatfomr_shim_brcm97252] %s:%d \n", __FUNCTION__, __LINE__);
    switch (attribute) {
    case SHIM_WINDOW_WIDTH:
      *value = kDefaultWidth;
      return true;
    case SHIM_WINDOW_HEIGHT:
      *value = kDefaultHeight;
      return true;
    default:
      return false;
    }
}

bool ShimDestroyWindow(ShimNativeWindowId window_id) {
    fprintf(stdout, "[eglplatfomr_shim_brcm97252] %s:%d \n", __FUNCTION__, __LINE__);
    if (gDFBSurface)    {
        DFBCHECK_RET(gDFBSurface->Release(gDFBSurface));
        gDFBSurface = NULL;
    }
    return true;
}

ShimEGLNativeDisplayType ShimGetNativeDisplay(void) {
    fprintf(stdout, "[eglplatfomr_shim_brcm97252] %s:%d returning %p\n",
            __FUNCTION__, __LINE__, (void*)(reinterpret_cast<ShimEGLNativeDisplayType>(EGL_DEFAULT_DISPLAY)));
    return reinterpret_cast<ShimEGLNativeDisplayType>(EGL_DEFAULT_DISPLAY);
}

ShimEGLNativeWindowType ShimGetNativeWindow(
    ShimNativeWindowId native_window_id) {

    if (!gDFBSurface) {
        ShimCreateWindow();
    }

    fprintf(stdout, "[eglplatfomr_shim_brcm97252] %s:%d returning %p\n",
            __FUNCTION__, __LINE__, (void*)(reinterpret_cast<ShimEGLNativeWindowType>(gDFBSurface)));
    return reinterpret_cast<ShimEGLNativeWindowType>(gDFBSurface);
}

bool ShimReleaseNativeWindow(ShimEGLNativeWindowType native_window) {
    fprintf(stdout, "[eglplatfomr_shim_brcm97252] %s:%d \n", __FUNCTION__, __LINE__);
    if (gDFB)                       DFBCHECK_RET(gDFB->Release(gDFB));
    return true;
}

#ifdef __cplusplus
}
#endif
