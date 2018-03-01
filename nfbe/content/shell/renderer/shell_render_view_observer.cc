// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/renderer/shell_render_view_observer.h"

#include "base/command_line.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/render_view_observer.h"
#include "content/shell/common/shell_switches.h"
#include "third_party/WebKit/public/web/WebTestingSupport.h"
#include "third_party/WebKit/public/web/WebView.h"

#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebDataSource.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/platform/WebURLError.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/platform/WebURLResponse.h"

#include "base/process/process_metrics.h"

#include <iomanip>

using namespace blink;


namespace content {

ShellRenderViewObserver::ShellRenderViewObserver(RenderView* render_view)
    : RenderViewObserver(render_view),
      statistics_output_(getenv("BLINK_STATISTICS_OUTPUT")) {
}

ShellRenderViewObserver::~ShellRenderViewObserver() {
}

void ShellRenderViewObserver::DidClearWindowObject(
    blink::WebLocalFrame* frame) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kExposeInternalsForTesting)) {
    blink::WebTestingSupport::injectInternalsObject(frame);
  }
}

void ShellRenderViewObserver::DidStartLoading() {
	if (statistics_output_) {
		LOG(INFO) << __FUNCTION__ << " @ " << base::Time::Now().ToJavaTime() << "ms";

		uintptr_t frameptr = 0;
		WebFrame* frame = render_view()->GetWebView()->mainFrame();
		if (frame) {
			frameptr = reinterpret_cast<uintptr_t>(frame);

			//store the start loading time stamp and the frame id
		    start_ts_map_[frameptr] = base::Time::Now().ToDoubleT();

			WebDataSource* ds = frame->dataSource();
			DCHECK(ds);

			if (ds) {
				const WebURLRequest& request = ds->request();
				LOG(INFO) << "<" << frameptr << "> Request URL : " << request.url();
			} else {
				LOG(INFO) << "<" << frameptr << "> Base URL    : " << frame->document().baseURL().string().utf8();
			}
		} else {
			LOG(INFO) << "<" << frameptr << "> Base URL    : UNKNOWN" ;
		}

#if DISPLAY_PROCESS_WORKSET
        scoped_ptr<base::ProcessMetrics> pm(base::ProcessMetrics::CreateProcessMetrics(base::GetCurrentProcessHandle()));
        base::WorkingSetKBytes ws_usage;
        pm->GetWorkingSetKBytes(&ws_usage);
        LOG(INFO) << "<" << frameptr << "> Process used memory (private+shared)    : " << ws_usage.priv+ws_usage.shared << "KB";
#else
        LOG(INFO) << "<" << frameptr << "> System-wide used memory    : " << base::GetSystemCommitCharge() << "KB";
#endif
	}
}

void ShellRenderViewObserver::DidStopLoading() {
	if (statistics_output_) {
		LOG(INFO) << __FUNCTION__ << " @ " << base::Time::Now().ToJavaTime() << "ms";

		uintptr_t frameptr = 0;
		WebFrame* frame = render_view()->GetWebView()->mainFrame();
		if (frame) {
            frameptr = reinterpret_cast<uintptr_t>(frame);

			WebDataSource* ds = frame->dataSource();
			DCHECK(ds);

			if (ds) {
				const WebURLRequest& request = ds->request();
				LOG(INFO) << "<" << frameptr << "> Request URL : " << request.url();
			} else {
				LOG(INFO) << "<" << frameptr << "> Base URL    : " << frame->document().baseURL().string().utf8();
			}

			if (frameptr && start_ts_map_[frameptr] != 0) {
				double current_time = base::Time::Now().ToDoubleT();
				LOG(INFO) << "<" << frameptr << "> Loading Time : "
						<< std::fixed << std::setprecision(2)
						<< (current_time - start_ts_map_[frameptr]) << "s";
			} else {
				LOG(INFO) << "<" << frameptr << "> Loading Time : UNKNOWN ";
			}
			start_ts_map_[frameptr] = base::Time::Now().ToDoubleT();
		} else {
			LOG(INFO) << "<" << frameptr << "> Loading Time : UNKNOWN ";
		}
	}
}

void ShellRenderViewObserver::DidFailLoad(blink::WebLocalFrame* frame,
                         const blink::WebURLError& error) {

	// when fail loading print an error in the console regardless
	// of the state of the statistics_output_ variable
	LOG(INFO) << __FUNCTION__ << " @ " << base::Time::Now().ToJavaTime() << "ms";

	uintptr_t frameptr = 0;
	if (frame) {
        frameptr = reinterpret_cast<uintptr_t>(frame);
		WebDataSource* ds = frame->dataSource();
		DCHECK(ds);

		if (ds) {
			const WebURLRequest& request = ds->request();
			LOG(INFO) << "<" << frameptr << "> Request URL : " << request.url();
		} else {
			LOG(INFO) << "<" << frameptr << "> Base URL    : " << frame->document().baseURL().string().utf8();
		}
	}
	LOG(INFO) << "<" << frameptr << "> Error";
	LOG(INFO) << "\tCode            : " << error.reason;
	LOG(INFO) << "\tDomain          : " << error.domain.utf8();
	LOG(INFO) << "\tUnreachable URL : " << error.unreachableURL;
	LOG(INFO) << "\tDescription     : " << error.localizedDescription.utf8();

	if (statistics_output_) {
#if DISPLAY_PROCESS_WORKSET
            scoped_ptr<base::ProcessMetrics> pm(base::ProcessMetrics::CreateProcessMetrics(base::GetCurrentProcessHandle()));
            base::WorkingSetKBytes ws_usage;
            pm->GetWorkingSetKBytes(&ws_usage);
            LOG(INFO) << "<" << frameptr << "> Process used memory (private+shared)    : " << ws_usage.priv+ws_usage.shared << "KB";
#else
            LOG(INFO) << "<" << frameptr << "> System-wide used memory    : " << base::GetSystemCommitCharge() << "KB";
#endif
	}
}

void ShellRenderViewObserver::DidFinishLoad(blink::WebLocalFrame* frame) {
	if (statistics_output_) {
		LOG(INFO) << __FUNCTION__ << " @ " << base::Time::Now().ToJavaTime() << "ms";

		uintptr_t frameptr = 0;
		if (frame) {
            frameptr = reinterpret_cast<uintptr_t>(frame);
			WebDataSource* ds = frame->dataSource();
			DCHECK(ds);

			if (ds) {
				const WebURLRequest& request = ds->request();
				LOG(INFO) << "<" << frameptr << "> Request URL : " << request.url();
			} else {
				LOG(INFO) << "<" << frameptr << "> Base URL    : " << frame->document().baseURL().string().utf8();
			}
		} else {
			LOG(INFO) << "<" << frameptr << "> Base URL    : UNKNOWN" ;
		}

#if DISPLAY_PROCESS_WORKSET
        scoped_ptr<base::ProcessMetrics> pm(base::ProcessMetrics::CreateProcessMetrics(base::GetCurrentProcessHandle()));
        base::WorkingSetKBytes ws_usage;
        pm->GetWorkingSetKBytes(&ws_usage);
        LOG(INFO) << "<" << frameptr << "> Process used memory (private+shared)    : " << ws_usage.priv+ws_usage.shared << "KB";
#else
        LOG(INFO) << "<" << frameptr << "> System-wide used memory    : " << base::GetSystemCommitCharge() << "KB";
#endif
	}
}


void ShellRenderViewObserver::Navigate(const GURL& url) {
	if (statistics_output_) {
		LOG(INFO) << __FUNCTION__;

		uintptr_t frameptr = 0;
		WebFrame* frame = render_view()->GetWebView()->mainFrame();
		if (frame) {
            frameptr = reinterpret_cast<uintptr_t>(frame);
		}
		LOG(INFO) << "<" << frameptr << "> Navigate URL : " << url;
		// store the start loading time stamp and the frame id
		// copied from DocumentLoadTiming::setNavigationStart()
		start_ts_map_[frameptr] = base::Time::Now().ToDoubleT();
	}
}

}  // namespace content
