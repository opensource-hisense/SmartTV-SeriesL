// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/egltest/eglplatform_shim.h"

#include <string.h>
#include <EGL/egl.h>

#include <atomic>
#include <map>
#include <vector>


#ifdef __cplusplus
extern "C" {
#endif

// Prints out the EGL configurations available
// Adds some window creation testing during initialization
#define EGL_DEBUG 0


EGLNativeDisplayType g_native_display;
// Supporting only one native window
EGLNativeWindowType g_native_window = nullptr;

const int kDefaultX = 0;
const int kDefaultY = 0;
const int kDefaultWidth = 1920;
const int kDefaultHeight = 1080;
const int kDefaultBorderWidth = 0;

const char* ShimQueryString(int name) {
  switch (name) {
    case SHIM_EGL_LIBRARY:
      return "libEGL.so.1";
    case SHIM_GLES_LIBRARY:
      return "libGLESv2.so.2";
    default:
      return NULL;
  }
}

#if EGL_DEBUG
static const char* GetLastEGLErrorString() {
  EGLint error = eglGetError();
  switch (error) {
    case EGL_SUCCESS:
      return "EGL_SUCCESS";
    case EGL_BAD_ACCESS:
      return "EGL_BAD_ACCESS";
    case EGL_BAD_ALLOC:
      return "EGL_BAD_ALLOC";
    case EGL_BAD_ATTRIBUTE:
      return "EGL_BAD_ATTRIBUTE";
    case EGL_BAD_CONTEXT:
      return "EGL_BAD_CONTEXT";
    case EGL_BAD_CONFIG:
      return "EGL_BAD_CONFIG";
    case EGL_BAD_CURRENT_SURFACE:
      return "EGL_BAD_CURRENT_SURFACE";
    case EGL_BAD_DISPLAY:
      return "EGL_BAD_DISPLAY";
    case EGL_BAD_SURFACE:
      return "EGL_BAD_SURFACE";
    case EGL_BAD_MATCH:
      return "EGL_BAD_MATCH";
    case EGL_BAD_PARAMETER:
      return "EGL_BAD_PARAMETER";
    case EGL_BAD_NATIVE_PIXMAP:
      return "EGL_BAD_NATIVE_PIXMAP";
    case EGL_BAD_NATIVE_WINDOW:
      return "EGL_BAD_NATIVE_WINDOW";
    default:
      return "UNKNOWN";
  }
}

static inline void
PrintEGLConfigAttribute(
    EGLDisplay in_egl_display,
    EGLConfig in_egl_config,
    EGLint in_attribute_id,
    const char *in_attribute_name,
    const char *in_format,
    EGLint *out_value) {
  ::eglGetConfigAttrib(in_egl_display, in_egl_config,
                       in_attribute_id, out_value);
  fprintf(stdout, in_format, in_attribute_name, *out_value);
}

#define PRINT_EGLCONFIG_ATTRIB_DEC(in_attrib, in_egl_config, out_value) \
  PrintEGLConfigAttribute(in_egl_display, in_egl_config, in_attrib, \
                          #in_attrib, \
                          "[eglplatform_shim_arm_a5] %s = %d\n", out_value)
#define PRINT_EGLCONFIG_ATTRIB_HEX(in_attrib, in_egl_config, out_value) \
  PrintEGLConfigAttribute(in_egl_display, in_egl_config, in_attrib, \
                          #in_attrib, \
                          "[eglplatform_shim_arm_a5] %s = 0x%08X\n", \
                          out_value)


static void
ListEGLConfigAttributes(
    EGLDisplay in_egl_display,
    EGLConfig *in_egl_config_list,
    int in_egl_config_num) {
  int idx;
  EGLConfig *config;
  EGLint value;

  for (idx = 0; idx < in_egl_config_num; ++idx) {
    config = in_egl_config_list + idx;

    fprintf(stdout, "[eglplatform_shim_arm_a5] "
                    "**********\nEGLConfig[%d] [%p]\n", idx, config);

    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_CONFIG_ID, *config, &value);

    PRINT_EGLCONFIG_ATTRIB_HEX(EGL_CONFORMANT, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_HEX(EGL_RENDERABLE_TYPE, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_HEX(EGL_SURFACE_TYPE, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_HEX(EGL_COLOR_BUFFER_TYPE, *config, &value);

    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_BUFFER_SIZE, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_RED_SIZE, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_BLUE_SIZE, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_GREEN_SIZE, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_ALPHA_SIZE, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_DEPTH_SIZE, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_STENCIL_SIZE, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_HEX(EGL_ALPHA_FORMAT, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_ALPHA_MASK_SIZE, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_LEVEL, *config, &value);

    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_MAX_PBUFFER_WIDTH, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_MAX_PBUFFER_HEIGHT, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_MAX_PBUFFER_PIXELS, *config, &value);

    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_MAX_SWAP_INTERVAL, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_MIN_SWAP_INTERVAL, *config, &value);

    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_NATIVE_VISUAL_ID, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_NATIVE_VISUAL_TYPE, *config, &value);

    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_SAMPLE_BUFFERS, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_SAMPLES, *config, &value);

    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_TRANSPARENT_RED_VALUE, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_TRANSPARENT_GREEN_VALUE, *config, &value);
    PRINT_EGLCONFIG_ATTRIB_DEC(EGL_TRANSPARENT_BLUE_VALUE, *config, &value);

    fprintf(stdout, "[eglplatform_shim_arm_a5] **********\n");
  }
}

// This function simulates roughly the calls done by the NFBE core
// in gl_surface_egl.cc
static bool TestEGLCreateSurface() {
  EGLDisplay egl_display = ::eglGetDisplay(g_native_display);
  EGLBoolean ebret = ::eglInitialize(egl_display, NULL, NULL);
  if (!ebret) {
    fprintf(stdout, "[eglplatform_shim_arm_a5] eglGetConfigs() "
        "failed with %s\n", GetLastEGLErrorString());
    return false;
  }
  EGLConfig config_list[32];
  EGLint config_num = 0;
  if (!::eglGetConfigs(egl_display, config_list,
                       sizeof(config_list) / sizeof(config_list[0]),
                       &config_num)) {
    fprintf(stdout, "eglGetConfigs() failed 0x%08x", eglGetError());
    return false;
  }
  ListEGLConfigAttributes(egl_display, config_list, config_num);

  EGLint config_attribs_8888[] = {
    EGL_BUFFER_SIZE, 32,
    EGL_ALPHA_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
//      EGL_SURFACE_TYPE, EGL_DONT_CARE,
    EGL_NONE
  };
  EGLint num_configs;
  EGLConfig config = nullptr;
  if (!eglChooseConfig(egl_display,
                       config_attribs_8888,
                       &config,
                       1,
                       &num_configs)) {
    fprintf(stdout, "[eglplatform_shim_arm_a5] eglChooseConfig "
        "failed with %s \n",
        GetLastEGLErrorString());
    return false;
  } else {
    fprintf(stdout, "[eglplatform_shim_arm_a5] eglChooseConfig "
        "returned with %d configs, selected %p \n", num_configs, config);
    ListEGLConfigAttributes(egl_display, &config, num_configs);
  }
  std::vector<EGLint> egl_window_attributes;

  uintptr_t window_id =
      reinterpret_cast<uintptr_t>(ShimCreateWindow());

  EGLNativeWindowType egl_window = nullptr;
  if (!(egl_window =
      reinterpret_cast<EGLNativeWindowType>(ShimGetNativeWindow(window_id)))) {
    fprintf(stdout, "[eglplatform_shim_arm_a5] ShimCreateWindow "
        "failed \n");
    return false;
  }

  const char* g_egl_extensions = eglQueryString(egl_display, EGL_EXTENSIONS);
  fprintf(stdout, "[eglplatform_shim_arm_a5] eglQueryString "
      "returned %s \n",
      g_egl_extensions);

  egl_window_attributes.push_back(EGL_NONE);
  // Create a surface for the native window.
  EGLSurface egl_surface = eglCreateWindowSurface(
      egl_display, config, egl_window, &egl_window_attributes[0]);

  if (!egl_surface) {
    fprintf(stdout, "[eglplatform_shim_arm_a5] eglCreateWindowSurface "
        "failed with error %s\n",
         GetLastEGLErrorString());
    return false;
  } else {
    fprintf(stdout, "[eglplatform_shim_arm_a5] eglCreateWindowSurface "
        "returned %p \n",
        egl_surface);
  }
  return true;
}


#endif  // EGL_DEBUG


bool ShimInitialize(void) {
  fprintf(stdout, "[eglplatform_shim_arm_a5] %s:%d \n", __FUNCTION__, __LINE__);
  g_native_display = (EGLNativeDisplayType)EGL_DEFAULT_DISPLAY;
#if EGL_DEBUG
  if (!TestEGLCreateSurface())
    return false;
#endif  // EGL_DEBUG
  return true;
}

bool ShimTerminate(void) {
  fprintf(stdout, "[eglplatform_shim_arm_a5] %s:%d \n", __FUNCTION__, __LINE__);
  return true;
}

ShimNativeWindowId ShimCreateWindow(void) {
  fprintf(stdout, "[eglplatform_shim_arm_a5] %s:%d \n", __FUNCTION__, __LINE__);
  // This function is called from the browser process to get an ID
  // not to actually create the native window !!!
  // The native window is created in the GPU process by the
  // ShimGetNativeWindow() below.

  // Supporting only one native window with the ID 1.
  uint32_t native_window_id = 1;

  fprintf(stdout, "[eglplatform_shim_arm_a5] %s:%d returning %u\n",
          __FUNCTION__, __LINE__,
          reinterpret_cast<ShimNativeWindowId>(native_window_id));
  return reinterpret_cast<ShimNativeWindowId>(native_window_id);
}

bool ShimQueryWindow(ShimNativeWindowId window_id, int attribute, int* value) {
  fprintf(stdout, "[eglplatform_shim_arm_a5] %s:%d \n", __FUNCTION__, __LINE__);
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
  fprintf(stdout, "[eglplatform_shim_arm_a5] %s:%d called with ID %u\n",
          __FUNCTION__, __LINE__, window_id);
  // Nothing to do, we support anyway only one native window
  if (1 != window_id) {
    return false;
  }
  return true;
}

ShimEGLNativeDisplayType ShimGetNativeDisplay(void) {
  fprintf(stdout, "[eglplatform_shim_arm_a5] %s:%d returning %u\n",
          __FUNCTION__, __LINE__,
          reinterpret_cast<ShimEGLNativeDisplayType>(g_native_display));
  return reinterpret_cast<ShimEGLNativeDisplayType>(g_native_display);
}

ShimEGLNativeWindowType ShimGetNativeWindow(
    ShimNativeWindowId native_window_id) {
  fprintf(stdout, "[eglplatform_shim_arm_a5] %s:%d called with ID %d \n",
      __FUNCTION__, __LINE__, native_window_id);
  // currently supporting only one native window
  if (1 != native_window_id) {
    return 0;
  }

  if (!g_native_window) {
    // Creating a native window when demanded by the GPU process.
    // Especially because this is a pointer to a local structure which cannot
    // be passed between processes.
    fbdev_window* native_window = new fbdev_window;
    if (!native_window) {
          return 0;
    }
    native_window->width = kDefaultWidth;
    native_window->height = kDefaultHeight;
    g_native_window = native_window;
  }

  fprintf(stdout, "[eglplatform_shim_arm_a5] %s:%d returning %u\n",
          __FUNCTION__, __LINE__,
          reinterpret_cast<intptr_t>(g_native_window));
  return reinterpret_cast<intptr_t>(g_native_window);
}

bool ShimReleaseNativeWindow(ShimEGLNativeWindowType native_window) {
  fprintf(stdout, "[eglplatform_shim_arm_a5] %s:%d called with %u\n",
      __FUNCTION__, __LINE__, reinterpret_cast<intptr_t>(native_window));
  // currently supporting only one native window
  fbdev_window* native_window_ptr =
      reinterpret_cast<fbdev_window*>(native_window);
  if (!native_window_ptr || g_native_window != native_window_ptr) {
    return false;
  }

  delete native_window_ptr;
  g_native_window = nullptr;

  return true;
}

#ifdef __cplusplus
}
#endif
