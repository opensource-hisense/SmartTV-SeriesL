// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefsimple/simple_handler.h"

#include <sstream>
#include <string>
#include "include/base/cef_logging.h"
#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

namespace {

SimpleHandler* g_instance = NULL;

}  // namespace

SimpleHandler::SimpleHandler(bool use_views)
    : use_views_(use_views),
      is_closing_(false),
      statistics_output_(getenv("BLINK_STATISTICS_OUTPUT")) {
  DCHECK(!g_instance);
  g_instance = this;
#if defined(HBBTV_ON)
  // create or get a handler to the single application manager object
  cef_hbbtv_app_mgr_  = CefHBBTVAppMgr::Create();
#endif
}

SimpleHandler::~SimpleHandler() {
  g_instance = NULL;
}

// static
SimpleHandler* SimpleHandler::GetInstance() {
  return g_instance;
}

void SimpleHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                  const CefString& title) {
  CEF_REQUIRE_UI_THREAD();

  if (use_views_) {
    // Set the title of the window using the Views framework.
    CefRefPtr<CefBrowserView> browser_view =
        CefBrowserView::GetForBrowser(browser);
    if (browser_view) {
      CefRefPtr<CefWindow> window = browser_view->GetWindow();
      if (window)
        window->SetTitle(title);
    }
  } else {
    // Set the title of the window using platform APIs.
    PlatformTitleChange(browser, title);
  }
}

bool SimpleHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                             const CefString& message,
                             const CefString& source,
                             int line) {
  const int32_t resolved_level = cef::logging::LOG_INFO;
  cef::logging::LogMessage("CONSOLE", line, resolved_level).stream()
      << "\"" << message.ToString() << "\", source: " << source.ToString() <<
      " (" << line << ")";
  return true;
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Add to the list of existing browsers.
  browser_list_.push_back(browser);

#if defined(HBBTV_ON)
  // If there has not been any browser, or all the browser have been closed,
  // and command line has "--cef-inject-ait-test", inject this AIT update
  // test.
  {
    CefRefPtr<CefCommandLine> command_line =
        CefCommandLine::GetGlobalCommandLine();
    if (command_line->HasSwitch("cef-inject-ait-test")) {
      CefPostDelayedTask(TID_UI, base::Bind(&SimpleHandler::simulateAITUpdate,
                                            this, 1, true),
                         1000);
      CefPostDelayedTask(TID_UI, base::Bind(&SimpleHandler::simulateAITUpdate,
                                            this, 1, false),
                         5000);
      CefPostDelayedTask(TID_UI, base::Bind(&SimpleHandler::simulateAITUpdate,
                                            this, 2, true),
                         10000);
      CefPostDelayedTask(TID_UI, base::Bind(&SimpleHandler::simulateAITUpdate,
                                            this, 3, true),
                         15000);
    }
  }
#endif
}

bool SimpleHandler::DoClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Closing the main window requires special handling. See the DoClose()
  // documentation in the CEF header for a detailed destription of this
  // process.
  if (browser_list_.size() == 1) {
    // Set a flag to indicate that the window close should be allowed.
    is_closing_ = true;
  }

  // Allow the close. For windowed browsers this will result in the OS close
  // event being sent.
  return false;
}

void SimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Remove from the list of existing browsers.
  BrowserList::iterator bit = browser_list_.begin();
  for (; bit != browser_list_.end(); ++bit) {
    if ((*bit)->IsSame(browser)) {
      browser_list_.erase(bit);
      break;
    }
  }

  if (browser_list_.empty()) {
    // All browser windows have closed. Quit the application message loop.
    CefQuitMessageLoop();
  }
}

void SimpleHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) {
  CEF_REQUIRE_UI_THREAD();

  // Don't display an error for downloaded files.
  if (errorCode == ERR_ABORTED)
    return;

  // Display a load error message.
  std::stringstream ss;
  ss << "<html><body bgcolor=\"white\">"
        "<h2>Failed to load URL " << std::string(failedUrl) <<
        " with error " << std::string(errorText) << " (" << errorCode <<
        ").</h2></body></html>";
  frame->LoadString(ss.str(), failedUrl);
}

void SimpleHandler::CloseAllBrowsers(bool force_close) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute on the UI thread.
    CefPostTask(TID_UI,
        base::Bind(&SimpleHandler::CloseAllBrowsers, this, force_close));
    return;
  }

  if (browser_list_.empty())
    return;

  BrowserList::const_iterator it = browser_list_.begin();
  for (; it != browser_list_.end(); ++it)
    (*it)->GetHost()->CloseBrowser(force_close);
}

#if defined(HBBTV_ON)
CefHBBTVAppMgrPtr SimpleHandler::GetHBBTVAppMgr() {
    if (!cef_hbbtv_app_mgr_)
        return NULL;
    return cef_hbbtv_app_mgr_->GetHBBTVAppMgr();
}

bool SimpleHandler::OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                             const CefKeyEvent& event,
                             CefEventHandle os_event,
                             bool* is_keyboard_shortcut) {
    if (!cef_hbbtv_app_mgr_)
        return false;
    return cef_hbbtv_app_mgr_->PreHandleKeyboardEvent(event, os_event);
}

void SimpleHandler::simulateAITUpdate(int in_index, bool in_isChannelChange) {
  std::vector<char*> v;

  if (in_index == 1) {
    const char* start_uri_ = "http://www.google.com";

    cef_dvb_application_transport_protocol_t tpTest1_[8];
    cef_hbbtv_dvb_application_information_t aitTest1_[14];

    memset(tpTest1_, 0, sizeof tpTest1_);
    memset(aitTest1_, 0, sizeof aitTest1_);

    tpTest1_[0].fBaseURL = simulateAITUpdateStrdupHelper(v, start_uri_);
    tpTest1_[0].fProtocolID = DVBAPPLICATION_PROTOCOL_HTTP;
    tpTest1_[1].fBaseURL = simulateAITUpdateStrdupHelper(
        v, "http://itv.mit-xperts.com/");
    tpTest1_[1].fProtocolID = DVBAPPLICATION_PROTOCOL_HTTP;
    tpTest1_[2].fBaseURL = simulateAITUpdateStrdupHelper(
        v, "http://hbbtv.ardmediathek.de/");
    tpTest1_[2].fProtocolID = DVBAPPLICATION_PROTOCOL_HTTP;
    tpTest1_[3].fBaseURL = simulateAITUpdateStrdupHelper(
        v, "http://itv.ard.de/");
    tpTest1_[3].fProtocolID = DVBAPPLICATION_PROTOCOL_HTTP;
    tpTest1_[4].fBaseURL = simulateAITUpdateStrdupHelper(
        v, "http://hbbtv.daserste.de/");
    tpTest1_[4].fProtocolID = DVBAPPLICATION_PROTOCOL_HTTP;
    tpTest1_[5].fBaseURL = simulateAITUpdateStrdupHelper(
        v, "http://hbbtv.zdf.de/");
    tpTest1_[5].fProtocolID = DVBAPPLICATION_PROTOCOL_HTTP;
    tpTest1_[6].fBaseURL = simulateAITUpdateStrdupHelper(
        v, "http://itv2.ard.de/");
    tpTest1_[6].fProtocolID = DVBAPPLICATION_PROTOCOL_HTTP;
    // FIXME: is this [7]? but [7] is not used....
    tpTest1_[6].fBaseURL = simulateAITUpdateStrdupHelper(
        v, "dvb://current.ait/13.1f7");
    tpTest1_[6].fProtocolID = DVBAPPLICATION_PROTOCOL_DSMCC;

    aitTest1_[0].fApplicationID = 0;
    aitTest1_[0].fOrganisationID = 0;
    aitTest1_[0].fLocation = simulateAITUpdateStrdupHelper(v, "");
    aitTest1_[0].fTransportProtocols = &tpTest1_[0];
    aitTest1_[0].fTransportProtocolsCount = 1;
    aitTest1_[0].fPriority = 0;
    aitTest1_[0].fControlCode = DVBAPPLICATION_CONTROLCODE_AUTOSTART;
    aitTest1_[0].fUsageType = OIPFAPPLICATION_USAGE_TYPE_GENERIC;

    aitTest1_[1].fApplicationID = 0x1f5;
    aitTest1_[1].fOrganisationID = 19;
    aitTest1_[1].fLocation = simulateAITUpdateStrdupHelper(
        v, "hbbtvtest/appmanager/otherapp.php?param1=value1");
    aitTest1_[1].fTransportProtocols = &tpTest1_[1];
    aitTest1_[1].fTransportProtocolsCount = 1;
    aitTest1_[1].fPriority = 0;
    aitTest1_[1].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[1].fUsageType = OIPFAPPLICATION_USAGE_TYPE_TELETEXT;
    aitTest1_[2].fApplicationID = 0x3;
    aitTest1_[2].fOrganisationID = 19;
    aitTest1_[2].fLocation = simulateAITUpdateStrdupHelper(
        v, "hbbtv-ard/mediathek/");
    aitTest1_[2].fTransportProtocols = &tpTest1_[2];
    aitTest1_[2].fTransportProtocolsCount = 1;
    aitTest1_[2].fPriority = 0;
    aitTest1_[2].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[2].fUsageType = OIPFAPPLICATION_USAGE_TYPE_TELETEXT;
    aitTest1_[3].fApplicationID = 0xa;
    aitTest1_[3].fOrganisationID = 19;
    aitTest1_[3].fLocation = simulateAITUpdateStrdupHelper(v, "hbbtvtest/");
    aitTest1_[3].fTransportProtocols = &tpTest1_[1];
    aitTest1_[3].fTransportProtocolsCount = 1;
    aitTest1_[3].fPriority = 0;
    aitTest1_[3].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[3].fUsageType = OIPFAPPLICATION_USAGE_TYPE_TELETEXT;

    aitTest1_[4].fApplicationID = 0x2;
    aitTest1_[4].fOrganisationID = 19;
    aitTest1_[4].fLocation = simulateAITUpdateStrdupHelper(v, "ardepg/");
    aitTest1_[4].fTransportProtocols = &tpTest1_[3];
    aitTest1_[4].fTransportProtocolsCount = 1;
    aitTest1_[4].fPriority = 0;
    aitTest1_[4].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[4].fUsageType = OIPFAPPLICATION_USAGE_TYPE_TELETEXT;

    aitTest1_[5].fApplicationID = 0x14;
    aitTest1_[5].fOrganisationID = 19;
    aitTest1_[5].fLocation = simulateAITUpdateStrdupHelper(v, "");
    aitTest1_[5].fTransportProtocols = &tpTest1_[4];
    aitTest1_[5].fTransportProtocolsCount = 1;
    aitTest1_[5].fPriority = 0;
    aitTest1_[5].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[5].fUsageType = OIPFAPPLICATION_USAGE_TYPE_TELETEXT;

    aitTest1_[6].fApplicationID = 0x1;
    aitTest1_[6].fOrganisationID = 0x11;
    aitTest1_[6].fLocation = simulateAITUpdateStrdupHelper(
        v, "zdfstart/index.php");
    aitTest1_[6].fTransportProtocols = &tpTest1_[1];
    aitTest1_[6].fTransportProtocolsCount = 1;
    aitTest1_[6].fPriority = 0;
    aitTest1_[6].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[6].fUsageType = OIPFAPPLICATION_USAGE_TYPE_GENERIC;

    aitTest1_[7].fApplicationID = 0x2;
    aitTest1_[7].fOrganisationID = 17;
    aitTest1_[7].fLocation = simulateAITUpdateStrdupHelper(
        v, "zdfmediathek/");
    aitTest1_[7].fTransportProtocols = &tpTest1_[5];
    aitTest1_[7].fTransportProtocolsCount = 1;
    aitTest1_[7].fPriority = 0;
    aitTest1_[7].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[7].fUsageType = OIPFAPPLICATION_USAGE_TYPE_GENERIC;

    aitTest1_[8].fApplicationID = 6;  // blue
    aitTest1_[8].fOrganisationID = 19;
    aitTest1_[8].fLocation = simulateAITUpdateStrdupHelper(
        v, "zdfstart/index.php");
    aitTest1_[8].fTransportProtocols = &tpTest1_[1];
    aitTest1_[8].fTransportProtocolsCount = 1;
    aitTest1_[8].fPriority = 0;
    aitTest1_[8].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[8].fUsageType = OIPFAPPLICATION_USAGE_TYPE_GENERIC;

    aitTest1_[9].fApplicationID = 1;  // 0
    aitTest1_[9].fOrganisationID = 19;
    aitTest1_[9].fLocation = simulateAITUpdateStrdupHelper(
        v, "ardstart/index.php");
    aitTest1_[9].fTransportProtocols = &tpTest1_[3];
    aitTest1_[9].fTransportProtocolsCount = 1;
    aitTest1_[9].fPriority = 0;
    aitTest1_[9].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[9].fUsageType = OIPFAPPLICATION_USAGE_TYPE_GENERIC;

    aitTest1_[10].fApplicationID = 17;  // yellow
    aitTest1_[10].fOrganisationID = 19;
    aitTest1_[10].fLocation = simulateAITUpdateStrdupHelper(
        v, "hbbtv-ard/mediathek/");
    aitTest1_[10].fTransportProtocols = &tpTest1_[2];
    aitTest1_[10].fTransportProtocolsCount = 1;
    aitTest1_[10].fPriority = 0;
    aitTest1_[10].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[10].fUsageType = OIPFAPPLICATION_USAGE_TYPE_TELETEXT;

    aitTest1_[11].fApplicationID = 4;  // blue
    aitTest1_[11].fOrganisationID = 19;
    aitTest1_[11].fLocation = simulateAITUpdateStrdupHelper(v, "ardtext");
    aitTest1_[11].fTransportProtocols = &tpTest1_[6];
    aitTest1_[11].fTransportProtocolsCount = 1;
    aitTest1_[11].fPriority = 0;
    aitTest1_[11].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[11].fUsageType = OIPFAPPLICATION_USAGE_TYPE_GENERIC;

    aitTest1_[11].fApplicationID = 4;  // blue
    aitTest1_[11].fOrganisationID = 19;
    aitTest1_[11].fLocation = simulateAITUpdateStrdupHelper(v, "ardtext");
    aitTest1_[11].fTransportProtocols = &tpTest1_[6];
    aitTest1_[11].fTransportProtocolsCount = 1;
    aitTest1_[11].fPriority = 0;
    aitTest1_[11].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[11].fUsageType = OIPFAPPLICATION_USAGE_TYPE_GENERIC;

    aitTest1_[12].fApplicationID = 3;  // heute journal plus
    aitTest1_[12].fOrganisationID = 17;
    aitTest1_[12].fLocation = simulateAITUpdateStrdupHelper(v, "zdfhjplus");
    aitTest1_[12].fTransportProtocols = &tpTest1_[5];
    aitTest1_[12].fTransportProtocolsCount = 1;
    aitTest1_[12].fPriority = 0;
    aitTest1_[12].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[12].fUsageType = OIPFAPPLICATION_USAGE_TYPE_GENERIC;

    aitTest1_[13].fApplicationID = 0x1f7;  // test 8 starting from DSMCC
    aitTest1_[13].fOrganisationID = 19;
    aitTest1_[13].fLocation = simulateAITUpdateStrdupHelper(v, "");
    aitTest1_[13].fTransportProtocols = &tpTest1_[6];
    aitTest1_[13].fTransportProtocolsCount = 1;
    aitTest1_[13].fPriority = 0;
    aitTest1_[13].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest1_[13].fUsageType = OIPFAPPLICATION_USAGE_TYPE_GENERIC;

    cef_hbbtv_app_mgr_->OnAITUpdate(aitTest1_, 14, in_isChannelChange, false);
  } else if (in_index == 2) {
    cef_dvb_application_transport_protocol_t tpTest2_[5];
    cef_hbbtv_dvb_application_information_t aitTest2_[5];

    memset(tpTest2_, 0, sizeof tpTest2_);
    memset(aitTest2_, 0, sizeof aitTest2_);

    tpTest2_[0].fBaseURL = simulateAITUpdateStrdupHelper(
        v, "http://itv-origin.ard.de/ardstart/");
    tpTest2_[0].fProtocolID = DVBAPPLICATION_PROTOCOL_HTTP;
    tpTest2_[1].fBaseURL = simulateAITUpdateStrdupHelper(
        v, "http://hbbtv.ardmediathek.de/");
    tpTest2_[1].fProtocolID = DVBAPPLICATION_PROTOCOL_HTTP;
    tpTest2_[2].fBaseURL = simulateAITUpdateStrdupHelper(
        v, "http://itv.ard.de/");
    tpTest2_[2].fProtocolID = DVBAPPLICATION_PROTOCOL_HTTP;
    tpTest2_[3].fBaseURL = simulateAITUpdateStrdupHelper(
        v, "http://hbbtv.daserste.de/");
    tpTest2_[3].fProtocolID = DVBAPPLICATION_PROTOCOL_HTTP;
    tpTest2_[4].fBaseURL = simulateAITUpdateStrdupHelper(
        v, "http://itv2.ard.de/");
    tpTest2_[4].fProtocolID = DVBAPPLICATION_PROTOCOL_HTTP;

    aitTest2_[0].fApplicationID = 1;
    aitTest2_[0].fOrganisationID = 19;
    aitTest2_[0].fVisibility = DVB_VISIBLE_ALL;
    aitTest2_[0].fPriority = 1;
    aitTest2_[0].fControlCode = DVBAPPLICATION_CONTROLCODE_AUTOSTART;
    aitTest2_[0].fTransportProtocolsCount = 1;
    aitTest2_[0].fTransportProtocols = &tpTest2_[0];
    aitTest2_[0].fIsServiceBound = 1;
    aitTest2_[0].fLocation = simulateAITUpdateStrdupHelper(v, "index.html");
    aitTest2_[0].fBoundariesCount = 0;
    aitTest2_[0].fNamesCount = 0;
    aitTest2_[0].fIconFlags = 0;
    aitTest2_[0].fUsageType = OIPFAPPLICATION_USAGE_TYPE_GENERIC;

    aitTest2_[1].fApplicationID = 0x3;
    aitTest2_[1].fOrganisationID = 19;
    aitTest2_[1].fVisibility = DVB_VISIBLE_ALL;
    aitTest2_[1].fPriority = 0;
    aitTest2_[1].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest2_[1].fTransportProtocolsCount = 1;
    aitTest2_[1].fTransportProtocols = &tpTest2_[1];
    aitTest2_[1].fIsServiceBound = 1;
    aitTest2_[1].fLocation = simulateAITUpdateStrdupHelper(
        v, "hbbtv-ard/mediathek/");
    aitTest2_[1].fBoundariesCount = 0;
    aitTest2_[1].fNamesCount = 0;
    aitTest2_[1].fIconFlags = 0;
    aitTest2_[1].fUsageType = OIPFAPPLICATION_USAGE_TYPE_TELETEXT;

    aitTest2_[2].fApplicationID = 0x2;
    aitTest2_[2].fOrganisationID = 19;
    aitTest2_[2].fVisibility = DVB_VISIBLE_ALL;
    aitTest2_[2].fPriority = 0;
    aitTest2_[2].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest2_[2].fTransportProtocolsCount = 1;
    aitTest2_[2].fTransportProtocols = &tpTest2_[2];
    aitTest2_[2].fIsServiceBound = 1;
    aitTest2_[2].fLocation = simulateAITUpdateStrdupHelper(v, "ardepg/");
    aitTest2_[2].fBoundariesCount = 0;
    aitTest2_[2].fNamesCount = 0;
    aitTest2_[2].fIconFlags = 0;
    aitTest2_[2].fUsageType = OIPFAPPLICATION_USAGE_TYPE_TELETEXT;

    aitTest2_[3].fApplicationID = 0x14;
    aitTest2_[3].fOrganisationID = 19;
    aitTest2_[3].fVisibility = DVB_VISIBLE_ALL;
    aitTest2_[3].fPriority = 0;
    aitTest2_[3].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest2_[3].fTransportProtocolsCount = 1;
    aitTest2_[3].fTransportProtocols = &tpTest2_[3];
    aitTest2_[3].fLocation = simulateAITUpdateStrdupHelper(v, "");
    aitTest2_[3].fBoundariesCount = 0;
    aitTest2_[3].fNamesCount = 0;
    aitTest2_[3].fIconFlags = 0;
    aitTest2_[3].fUsageType = OIPFAPPLICATION_USAGE_TYPE_TELETEXT;

    aitTest2_[4].fApplicationID = 0x4;  // blue
    aitTest2_[4].fOrganisationID = 19;
    aitTest2_[4].fVisibility = DVB_VISIBLE_ALL;
    aitTest2_[4].fPriority = 0;
    aitTest2_[4].fControlCode = DVBAPPLICATION_CONTROLCODE_PRESENT;
    aitTest2_[4].fTransportProtocolsCount = 1;
    aitTest2_[4].fTransportProtocols = &tpTest2_[4];
    aitTest2_[4].fLocation = simulateAITUpdateStrdupHelper(v, "ardtext");
    aitTest2_[4].fBoundariesCount = 0;
    aitTest2_[4].fNamesCount = 0;
    aitTest2_[4].fIconFlags = 0;
    aitTest2_[4].fUsageType = OIPFAPPLICATION_USAGE_TYPE_GENERIC;

    cef_hbbtv_app_mgr_->OnAITUpdate(aitTest2_, 5, in_isChannelChange, false);
  } else if (in_index == 3) {
    cef_dvb_application_transport_protocol_t tpTest3_[1];
    cef_hbbtv_dvb_application_information_t aitTest3_[1];

    memset(tpTest3_, 0, sizeof tpTest3_);
    memset(aitTest3_, 0, sizeof aitTest3_);

    tpTest3_[0].fBaseURL = simulateAITUpdateStrdupHelper(
        v, "http://itv-origin.ard.de/ardstart/");
    tpTest3_[0].fProtocolID = DVBAPPLICATION_PROTOCOL_HTTP;

    aitTest3_[0].fApplicationID = 1;
    aitTest3_[0].fOrganisationID = 9;
    aitTest3_[0].fVisibility = DVB_VISIBLE_ALL;
    aitTest3_[0].fPriority = 1;
    aitTest3_[0].fControlCode = DVBAPPLICATION_CONTROLCODE_AUTOSTART;
    aitTest3_[0].fTransportProtocolsCount = 1;
    aitTest3_[0].fTransportProtocols = &tpTest3_[0];
    aitTest3_[0].fIsServiceBound = 1;
    aitTest3_[0].fLocation = simulateAITUpdateStrdupHelper(v, "index.html");
    aitTest3_[0].fBoundariesCount = 0;
    aitTest3_[0].fNamesCount = 0;
    aitTest3_[0].fIconFlags = 0;
    aitTest3_[0].fUsageType = OIPFAPPLICATION_USAGE_TYPE_GENERIC;

    cef_hbbtv_app_mgr_->OnAITUpdate(aitTest3_, 1, in_isChannelChange, false);
  }

  for (std::vector<char*>::iterator it = v.begin(); it != v.end(); it++) {
    if (*it) {
      free(*it);
      *it = NULL;
    }
  }
  v.clear();
}

char* SimpleHandler::simulateAITUpdateStrdupHelper(std::vector<char*>& v,
                                                   const char* s) {
  char* r = strdup(s);
  if (r)
    v.push_back(r);
  return r;
}

#endif  // defined(HBBTV_ON)
