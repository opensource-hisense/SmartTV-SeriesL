// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/*
    eglplatform_shim_ld6b.cc
    Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.
    All rights are reserved by ACCESS CO., LTD., whether the whole or
    part of the source code including any modifications.
*/

#include "ui/ozone/platform/egltest/eglplatform_shim.h"

#include <string.h>

#include <stdio.h>
#include <memmap/memmap.h>

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
    return (uint32_t)(DDR_PHYMEMADDR(DDR_LOSD0_0_OFFSET));
}

static EGLFormat getOSDFormat()
{
    return EGL_RGBA8888_COMPRESS;
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
