// Copyright (c) 2014 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefsimple/simple_handler.h"

#ifdef USE_X11
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#endif

#include <string>

#include "include/base/cef_logging.h"
#include "include/cef_browser.h"
#include "include/cef_trace.h"

void SimpleHandler::PlatformTitleChange(CefRefPtr<CefBrowser> browser,
                                        const CefString& title) {
#ifdef USE_X11
  std::string titleStr(title);

  // Retrieve the X11 display shared with Chromium.
  ::Display* display = cef_get_xdisplay();
  DCHECK(display);

  // Retrieve the X11 window handle for the browser.
  ::Window window = browser->GetHost()->GetWindowHandle();
  DCHECK(window != kNullWindowHandle);

  // Retrieve the atoms required by the below XChangeProperty call.
  const char* kAtoms[] = {
    "_NET_WM_NAME",
    "UTF8_STRING"
  };
  Atom atoms[2];
  int result = XInternAtoms(display, const_cast<char**>(kAtoms), 2, false,
                            atoms);
  if (!result)
    NOTREACHED();

  // Set the window title.
  XChangeProperty(display,
                  window,
                  atoms[0],
                  atoms[1],
                  8,
                  PropModeReplace,
                  reinterpret_cast<const unsigned char*>(titleStr.c_str()),
                  titleStr.size());

  // TODO(erg): This is technically wrong. So XStoreName and friends expect
  // this in Host Portable Character Encoding instead of UTF-8, which I believe
  // is Compound Text. This shouldn't matter 90% of the time since this is the
  // fallback to the UTF8 property above.
  XStoreName(display, browser->GetHost()->GetWindowHandle(), titleStr.c_str());
#endif
}

void SimpleHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                    bool isLoading,
                                    bool canGoBack,
                                    bool canGoForward) {
    if (statistics_output_) {
        int64 ts = CefNowFromSystemTraceTime() / 1000; // us to ms
        if (isLoading) {
            // the browser just started loading a new page
            LOG(INFO) << "Page progress: 0 @ " << ts << "ms";
        } else {
            // the browser just finished loading a page
            LOG(INFO) << "Page progress: 100 @ " << ts << "ms";
        }
        // TODO: Add memory consumption output
    }
}

void SimpleHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame) {
    if (statistics_output_) {
        int64 frameptr = frame->GetIdentifier();
        int64 ts = CefNowFromSystemTraceTime() / 1000; // us to ms
        LOG(INFO) << "LoadStart <" << frameptr << " @ " << ts << ">";

        if (frame->IsMain()) {
            //store the start loading time stamp and the frame id
            start_ts_map_[frameptr] = ts;
            // TODO: Add memory consumption output
        }
    }
}

void SimpleHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         int httpStatusCode) {
    if (statistics_output_) {
        int64 frameptr = frame->GetIdentifier();
        int64 ts = CefNowFromSystemTraceTime() / 1000; // us to ms

        LOG(INFO) << "LoadEnd <" << frameptr << " @ " << ts << "> Base URL[" << httpStatusCode << "]    : " << frame->GetURL().ToString();

        if (frame->IsMain()) {
            if (frameptr && start_ts_map_[frameptr] != 0) {
                int current_time = ts;
                LOG(INFO) << "<" << frameptr << "> Loading Time : "
                        << (current_time - start_ts_map_[frameptr]) << "ms";
            } else {
                LOG(INFO) << "<" << frameptr << "> Loading Time : UNKNOWN ";
            }
            // reset the time stamp in case it reloads
            start_ts_map_[frameptr] = ts;
            // TODO: Add memory consumption output
        }
    }
}
