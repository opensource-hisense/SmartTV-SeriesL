// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_RENDERER_SHELL_RENDER_VIEW_OBSERVER_H_
#define CONTENT_SHELL_RENDERER_SHELL_RENDER_VIEW_OBSERVER_H_

#include "base/macros.h"
#include <map>
#include <string>
#include "base/memory/scoped_ptr.h"
#include "content/public/renderer/render_view_observer.h"
#include "third_party/WebKit/public/platform/WebURL.h"

namespace blink {
class WebLocalFrame;
}

namespace content {

class RenderView;

// this switches between two display modes for memory consumption during start/finish loading
// if 1 the memory consumed by the current process is displayed
// - use this only in conjunction with --single-process
//   otherwise you get the memory consumption only for the browser process
// if 0 the total memory used from the system is displayed
// - this has the disadvantage that it counts the a part of the
//   memory allocated during this time by other processes too
// either one can calculate making the difference between the start memory and the finish loading memory
#define DISPLAY_PROCESS_WORKSET 1

typedef std::map<uintptr_t, double> ShellRenderViewTimeStampsMap;

class ShellRenderViewObserver : public RenderViewObserver {
 public:
  explicit ShellRenderViewObserver(RenderView* render_view);
  ~ShellRenderViewObserver() override;

 private:
  // RenderViewObserver implementation.
  void DidClearWindowObject(blink::WebLocalFrame* frame) override;

  void DidStartLoading() override;
  void DidStopLoading() override;
  void DidFailLoad(blink::WebLocalFrame* frame,
                           const blink::WebURLError& error) override;
  void DidFinishLoad(blink::WebLocalFrame* frame) override;

  void Navigate(const GURL& url) override;

  // stores the start loading time stamp for every frame ID
  ShellRenderViewTimeStampsMap start_ts_map_;

  // stores the value of the env var BLINK_STATISTICS_OUTPUT
  // to enable/disable the memory and page loading statistics print out
  bool statistics_output_;

  DISALLOW_COPY_AND_ASSIGN(ShellRenderViewObserver);
};

}  // namespace content

#endif  // CONTENT_SHELL_RENDERER_SHELL_RENDER_VIEW_OBSERVER_H_
