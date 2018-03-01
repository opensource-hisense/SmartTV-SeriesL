// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/*
    eglplatform_shim_ld11.cc
    Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.
    All rights are reserved by ACCESS CO., LTD., whether the whole or
    part of the source code including any modifications.
*/

#include "ui/ozone/platform/egltest/eglplatform_shim.h"

#include <string.h>

#include <stdio.h>
#include <memmap/memmap.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dtvdd/fb.h>
#define TARGET_FB   (0)

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>

#ifdef __cplusplus
extern "C" {
#endif

const int kDefaultWidth = 1920;
const int kDefaultHeight = 1080;

int g_width = kDefaultWidth;
int g_height = kDefaultHeight;
EGLNativeBufferType g_native_buffer;
EGLNativeBufferType* g_native = 0;

static uint32_t getOSDAddr()
{
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;
    char fbdev_name[15];
    int fd;
    int k;

    memset(fbdev_name, 0, 15);

    sprintf (fbdev_name, "/dev/dtv/fb%d", TARGET_FB);
    fprintf(stdout, "Open %s\n", fbdev_name);
    if ((fd = open (fbdev_name, O_RDWR)) < 0) {
        fprintf(stderr, "Error opening /dev/dtv/fb\n");
        return -1;
    }
    if ((k = ioctl (fd, FBIOGET_FSCREENINFO, &fix)) < 0) {
        fprintf(stderr, "Error with /dev/dtv/fb ioctl FBIOGET_FSCREENINFO");
        close (fd);
        return -2;
    }
    if ((k = ioctl (fd, FBIOGET_VSCREENINFO, &var)) < 0) {
        fprintf(stderr, "Error with /dev/dtv/fb ioctl FBIOGET_VSCREENINFO");
        close (fd);
        return -3;
    }
    return fix.smem_start;
}

static EGLFormat getOSDFormat()
{
    return EGL_BGRA8888;
}

const char* ShimQueryString(int name) {
    fprintf(stdout, "[eglplatfomr_shim_ld11] %s:%d \n", __FUNCTION__, __LINE__);
  switch (name) {
    case SHIM_EGL_LIBRARY:
      return "libEGL.so";
    case SHIM_GLES_LIBRARY:
      // on Broadcom platforms both EGL and GLES APIs are available
      // fromt the same library
      return "libGLESv2.so";
    default:
      return NULL;
  }
}

bool ShimInitialize(void) {
    fprintf(stdout, "[eglplatfomr_shim_ld11] %s:%d \n", __FUNCTION__, __LINE__);
    g_native_buffer.fbuf       = getOSDAddr();  //LOSD0_0
    g_native_buffer.format     = getOSDFormat();
    g_native_buffer.w          = g_width;
    g_native_buffer.h          = g_height;
    g_native_buffer.isPhysAddr = true;
    g_native = &g_native_buffer;

    return true;
}

bool ShimTerminate(void) {
    fprintf(stdout, "[eglplatfomr_shim_ld11] %s:%d \n", __FUNCTION__, __LINE__);

    g_native = 0;

    return true;
}

ShimNativeWindowId ShimCreateWindow(void) {
    fprintf(stdout, "[eglplatfomr_shim_ld11] %s:%d \n", __FUNCTION__, __LINE__);
    return (ShimNativeWindowId)g_native;
}

bool ShimQueryWindow(ShimNativeWindowId window_id, int attribute, int* value) {
    fprintf(stdout, "[eglplatfomr_shim_ld11] %s:%d \n", __FUNCTION__, __LINE__);
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
    fprintf(stdout, "[eglplatfomr_shim_ld11] %s:%d \n", __FUNCTION__, __LINE__);
    g_native = 0;
    return true;
}

ShimEGLNativeDisplayType ShimGetNativeDisplay(void) {
    fprintf(stdout, "[eglplatfomr_shim_ld11] %s:%d returning %p\n",
            __FUNCTION__, __LINE__, (void*)(reinterpret_cast<ShimEGLNativeDisplayType>(EGL_DEFAULT_DISPLAY)));
    return reinterpret_cast<ShimEGLNativeDisplayType>(EGL_DEFAULT_DISPLAY);
}

ShimEGLNativeWindowType ShimGetNativeWindow(
    ShimNativeWindowId native_window_id) {
    fprintf(stdout, "[eglplatfomr_shim_ld11] %s:%d \n", __FUNCTION__, __LINE__);
    if (!g_native) {
        (void)ShimCreateWindow();
    }
    return ShimEGLNativeWindowType(g_native);
}

bool ShimReleaseNativeWindow(ShimEGLNativeWindowType native_window) {
    fprintf(stdout, "[eglplatfomr_shim_ld11] %s:%d \n", __FUNCTION__, __LINE__);
    return true;
}

#ifdef __cplusplus
}
#endif
