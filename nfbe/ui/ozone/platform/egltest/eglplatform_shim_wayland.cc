// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/egltest/eglplatform_shim.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <wayland-client.h>
#include <wayland-egl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */

//FIXME: this is prone to ugly data races
static bool running = true;

const int MAX_EVENTS = 16;
// os-compatibility
extern "C" {
static int osEpollCreateCloExec(void);

static int setCloExecOrClose(int fd) {
  int flags;

  if (fd == -1)
    return -1;

  flags = fcntl(fd, F_GETFD);
  if (flags == -1)
    goto err;

  if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
    goto err;

  return fd;

  err:
    close(fd);
    return -1;
}

static int osEpollCreateCloExec(void) {
  int fd;

#ifdef EPOLL_CLOEXEC
  fd = epoll_create1(EPOLL_CLOEXEC);
  if (fd >= 0)
    return fd;
  if (errno != EINVAL)
    return -1;
#endif

  fd = epoll_create(1);
  return setCloExecOrClose(fd);
}
}  // os-compatibility

static void*
dispatch_thread_run(void* in_data)
{
    if (!in_data)
        return NULL;
    fprintf(stderr, "Starting dispatch thread ...\n");

    struct epoll_event ep[MAX_EVENTS];
    int i, ret, count = 0;
    uint32_t event = 0;
    bool epoll_err = false;
    unsigned int display_fd = wl_display_get_fd((struct wl_display*)in_data);
    int epoll_fd = osEpollCreateCloExec();
    if (epoll_fd < 0) {
        fprintf(stderr, "Epoll creation failed.");
        return NULL;
    }

    ep[0].events = EPOLLIN;
    ep[0].data.ptr = 0;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, display_fd, &ep[0]) < 0) {
      close(epoll_fd);
      fprintf(stderr, "epoll_ctl Add failed");
      return NULL;
    }

    // Adopted from:
    // http://cgit.freedesktop.org/wayland/weston/tree/clients/window.c#n5531.
    while (1) {
      wl_display_dispatch_pending((struct wl_display*)in_data);
      ret = wl_display_flush((struct wl_display*)in_data);
      if (ret < 0 && errno == EAGAIN) {
        ep[0].events = EPOLLIN | EPOLLERR | EPOLLHUP;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, display_fd, &ep[0]);
      } else if (ret < 0) {
        epoll_err = true;
        break;
      }
      // We have been asked to stop
      // polling. Break from the loop.
      if (!running)
        break;

//      fprintf(stderr, "Starting polling...\n");
      count = epoll_wait(epoll_fd, ep, MAX_EVENTS, -1);
      // Break if epoll wait returned value less than 0 and we aren't interrupted
      // by a signal.
      if (count < 0 && errno != EINTR) {
          fprintf(stderr, "epoll_wait returned an error %d.", errno);
        epoll_err = true;
        break;
      }

//      fprintf(stderr, "Got %d events...\n", count);

      for (i = 0; i < count; i++) {
        event = ep[i].events;
        // We can have cases where EPOLLIN and EPOLLHUP are both set for
        // example. Don't break if both flags are set.
        if (((event & EPOLLERR) || (event & EPOLLHUP)) &&
               !(event & EPOLLIN)) {
          epoll_err = true;
          break;
        }

        if (event & EPOLLIN) {
//          fprintf(stderr, "Dispatching event... %d \n", event);
          ret = wl_display_dispatch((struct wl_display*)in_data);
          if (ret == -1) {
            fprintf(stderr,  "wl_display_dispatch failed with an error.");
            epoll_err = true;
            break;
          }
        }
      }

//      fprintf(stderr, "Finished dispatching %d events...\n", count);

      if (epoll_err)
        break;
    }

    fprintf(stderr, "Exiting dispatch thread...\n");
    close(epoll_fd);
    return NULL;
}

/* -------------------------------------------------------------------- */

//const int kDefaultWidth = 1920;
//const int kDefaultHeight = 1080;
const int kDefaultWidth = 1280;
const int kDefaultHeight = 720;

struct wl_display *g_display = NULL;
struct wl_registry *g_registry = NULL;
struct wl_compositor *g_compositor = NULL;
struct wl_shell *g_shell = NULL;
int g_width = kDefaultWidth;
int g_height = kDefaultHeight;
struct wl_egl_window *g_native = NULL;
struct wl_surface *g_surface = NULL;
struct wl_shell_surface *g_shell_surface = NULL;

#if 1
#define SHIM_LOG(...) fprintf(stdout, "[eglplatform_shim_wayland:%d] %s:%d ", (int)getpid(), __FUNCTION__, __LINE__); \
                      fprintf(stdout,  __VA_ARGS__)
#else
#define SHIM_LOG(...)
#endif

const char* ShimQueryString(int name) {
  SHIM_LOG("\n");
  switch (name) {
    case SHIM_EGL_LIBRARY:
      return "libEGL.so.1";
    case SHIM_GLES_LIBRARY:
      return "libGLESv2.so";
    default:
      return NULL;
  }
}

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface ,uint32_t version) {
  if (strcmp(interface, "wl_compositor") == 0) {
    g_compositor = static_cast<struct wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
  } else if (strcmp(interface, "wl_shell") == 0) {
    g_shell = static_cast<struct wl_shell *>(wl_registry_bind(registry, name, &wl_shell_interface, 1));
  }
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
}

static const struct wl_registry_listener registry_listener = {
  registry_handle_global, registry_handle_global_remove
};

bool ShimInitialize(void) {
  SHIM_LOG("\n");
  if (g_display)
    return true;
  g_width = kDefaultWidth;
  g_height = kDefaultHeight;
  g_display = wl_display_connect(NULL);
  assert(g_display);
  g_registry = wl_display_get_registry(g_display);
  wl_registry_add_listener(g_registry, &registry_listener, NULL);
  wl_display_roundtrip(g_display);
  return g_display != NULL;
}

bool ShimTerminate(void) {
  SHIM_LOG("\n");
  running = false;
  if (g_registry) {
    wl_registry_destroy(g_registry);
    g_registry = NULL;
  }
  if (g_display) {
    wl_display_flush(g_display);
    wl_display_disconnect(g_display);
    g_display = NULL;
  }
  return true;
}

static void handle_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial) {
  wl_shell_surface_pong(shell_surface, serial);
}

static void handle_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height) {
  SHIM_LOG("g_native=%p width=%d height=%d\n", g_native, width, height);
  if (g_native)
    wl_egl_window_resize(g_native, width, height, 0, 0);
  g_width = width;
  g_height = height;
}

static void handle_popup_done(void *data, struct wl_shell_surface *shell_surface) {
  SHIM_LOG("\n");
}

static const struct wl_shell_surface_listener shell_surface_listener = {
  handle_ping,
  handle_configure,
  handle_popup_done
};

ShimNativeWindowId ShimCreateWindow(void) {
  SHIM_LOG("g_compositor=%p g_shell=%p\n", g_compositor, g_shell);
  assert(g_compositor);
  g_surface = wl_compositor_create_surface(g_compositor);
  assert(g_surface);
  assert(g_shell);
  g_shell_surface = wl_shell_get_shell_surface(g_shell, g_surface);
  assert(g_shell_surface);
  wl_shell_surface_add_listener(g_shell_surface, &shell_surface_listener, NULL);
  wl_shell_surface_set_title(g_shell_surface, "eglplatform_shim_wayland");
#if 1
  wl_shell_surface_set_fullscreen(g_shell_surface, WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 0, NULL);
#else
  wl_shell_surface_set_toplevel(g_shell_surface);
#endif
  g_native = wl_egl_window_create(g_surface, g_width, g_height);

  pthread_t dispatch_thread;
  int errcode;
  if ((errcode = pthread_create(&dispatch_thread, (pthread_attr_t *)NULL, dispatch_thread_run, (void*)(g_display)))) {
    fprintf(stderr, "%s() : Error creating progress thread %s !!\nExiting...\n", __FUNCTION__, strerror(errcode));
    return 0;
  }
  return (ShimNativeWindowId)g_native;
}

bool ShimQueryWindow(ShimNativeWindowId window_id, int attribute, int* value) {
  SHIM_LOG("\n");
  switch (attribute) {
    case SHIM_WINDOW_WIDTH:
      *value = g_width;
      return true;
    case SHIM_WINDOW_HEIGHT:
      *value = g_height;
      return true;
    default:
      return false;
  }
}

bool ShimDestroyWindow(ShimNativeWindowId window_id) {
  SHIM_LOG("g_native=%p g_shell_surface=%p g_surface=%p g_shell=%p g_compositor=%p\n", g_native, g_shell_surface, g_surface, g_shell, g_compositor);
  wl_egl_window_destroy(g_native);
  wl_shell_surface_destroy(g_shell_surface);
  wl_surface_destroy(g_surface);
  wl_shell_destroy(g_shell);
  wl_compositor_destroy(g_compositor);
  return true;
}

ShimEGLNativeDisplayType ShimGetNativeDisplay(void) {
  SHIM_LOG("g_display=%p\n", g_display);
  return reinterpret_cast<ShimEGLNativeDisplayType>(g_display);
}

ShimEGLNativeWindowType ShimGetNativeWindow(ShimNativeWindowId native_window_id) {
  SHIM_LOG("g_native=%p\n", g_native);
  if (!g_native) {
    (void)ShimCreateWindow();
  }
  return ShimEGLNativeWindowType(g_native);
}

bool ShimReleaseNativeWindow(ShimEGLNativeWindowType native_window) {
  SHIM_LOG("\n");
  return true;
}

#ifdef __cplusplus
}
#endif
