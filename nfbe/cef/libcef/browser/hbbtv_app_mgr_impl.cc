// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "libcef/browser/hbbtv_app_mgr_impl.h"

#if defined(HBBTV_ON)
#include "access/content/shell/browser/acs/hbbtv/hbbtv_app_mgr.h"
#include "include/cef_browser.h"
#endif

// static
std::unique_ptr<HBBTVAppMgrClassType> CefHBBTVAppMgrImpl::hbbtv_app_mgr_ = nullptr;
std::unique_ptr<HbbTVBrowserWindowFactoryClassType> CefHBBTVAppMgrImpl::factory_ = nullptr;

// static
CefRefPtr<CefHBBTVAppMgr> CefHBBTVAppMgr::Create() {
  return new CefHBBTVAppMgrImpl();
}

// overrides
cef_hbbtv_on_aitupdate_return_value_t CefHBBTVAppMgrImpl::OnAITUpdate(
    cef_hbbtv_dvb_application_information_t* in_ai_array, int in_count,
    bool in_isChannelChange, bool in_isXMLAIT) {
#if defined(HBBTV_ON)
  if (!hbbtv_app_mgr_)
    return ON_AITUPDATE_E_GENERIC;
  if (in_count <= 0)
    return ON_AITUPDATE_E_GENERIC;
  size_t tmp_size = (sizeof(content::hbbtv::TDVBApplicationInformation) *
                     in_count);
  void* vp = malloc(tmp_size);
  if (!vp)
    return ON_AITUPDATE_E_GENERIC;
  content::hbbtv::TDVBApplicationInformation* ai_array =
      static_cast<content::hbbtv::TDVBApplicationInformation*>(vp);
  memset(ai_array, 0, tmp_size);

  int rval = ON_AITUPDATE_E_GENERIC;

  const cef_hbbtv_dvb_application_information_t* in_ai = in_ai_array;

  content::hbbtv::TDVBApplicationInformation* ai = ai_array;
  const content::hbbtv::TDVBApplicationInformation* ai_endptr = ai + in_count;
  do {
    // check and bail out early to avoid unnecessary malloc.
    if (in_ai->fTransportProtocolsCount < 0 || in_ai->fBoundariesCount < 0 ||
        in_ai->fNamesCount < 0) {
      rval = ON_AITUPDATE_E_GENERIC;
      goto bailout_finish;
    }
    ai->fApplicationID = in_ai->fApplicationID;
    ai->fOrganisationID = in_ai->fOrganisationID;
    switch (in_ai->fVisibility) {
    case DVB_NOT_VISIBLE_ALL:
      ai->fVisibility =
          content::hbbtv::DVBApplicationVisibilityEnum::DVB_NOT_VISIBLE_ALL;
      break;
    case DVB_NOT_VISIBLE_USERS:
      ai->fVisibility =
          content::hbbtv::DVBApplicationVisibilityEnum::DVB_NOT_VISIBLE_USERS;
      break;
    case DVB_VISIBLE_ALL:
      ai->fVisibility =
          content::hbbtv::DVBApplicationVisibilityEnum::DVB_VISIBLE_ALL;
      break;
    default:
      rval = ON_AITUPDATE_E_GENERIC;
      goto bailout_finish;
    }
    ai->fPriority = in_ai->fPriority;
    switch (in_ai->fControlCode) {
    case DVBAPPLICATION_CONTROLCODE_AUTOSTART:
      ai->fControlCode =
          content::hbbtv::DVBApplicationControlCode::DVBAPPLICATION_CONTROLCODE_AUTOSTART;
      break;
    case DVBAPPLICATION_CONTROLCODE_PRESENT:
      ai->fControlCode =
          content::hbbtv::DVBApplicationControlCode::DVBAPPLICATION_CONTROLCODE_PRESENT;
      break;
    case DVBAPPLICATION_CONTROLCODE_KILL:
      ai->fControlCode =
          content::hbbtv::DVBApplicationControlCode::DVBAPPLICATION_CONTROLCODE_KILL;
      break;
    case DVBAPPLICATION_CONTROLCODE_DISABLED:
      ai->fControlCode =
          content::hbbtv::DVBApplicationControlCode::DVBAPPLICATION_CONTROLCODE_DISABLED;
      break;
    default:
      rval = ON_AITUPDATE_E_GENERIC;
      goto bailout_finish;
    }
    ai->fTransportProtocolsCount = in_ai->fTransportProtocolsCount;
    if (0 < in_ai->fTransportProtocolsCount) {
      tmp_size = (sizeof(content::hbbtv::TDVBApplicationTransportProtocol)
                  * in_ai->fTransportProtocolsCount);
      vp = malloc(tmp_size);
      if (!vp) {
        rval = ON_AITUPDATE_E_GENERIC;
        goto bailout_finish;
      }
      ai->fTransportProtocols =
          static_cast<content::hbbtv::TDVBApplicationTransportProtocol*>(vp);
      for (int j = 0; j < in_ai->fTransportProtocolsCount; j++) {
        switch (in_ai->fTransportProtocols[j].fProtocolID) {
        case DVBAPPLICATION_PROTOCOL_DSMCC:
          ai->fTransportProtocols[j].fProtocolID =
              content::hbbtv::DVBApplicationProtocol::DVBAPPLICATION_PROTOCOL_DSMCC;
          break;
        case DVBAPPLICATION_PROTOCOL_HTTP:
          ai->fTransportProtocols[j].fProtocolID =
              content::hbbtv::DVBApplicationProtocol::DVBAPPLICATION_PROTOCOL_HTTP;
          break;
        default:
          rval = ON_AITUPDATE_E_GENERIC;
          goto bailout_finish;
        }
        ai->fTransportProtocols[j].fBaseURL =
            in_ai->fTransportProtocols[j].fBaseURL;
      }
    }
    ai->fIsServiceBound = in_ai->fIsServiceBound;
    ai->fLocation = in_ai->fLocation;
    ai->fBoundariesCount = in_ai->fBoundariesCount;
    if (0 < in_ai->fBoundariesCount) {
      tmp_size = sizeof(char*) * in_ai->fBoundariesCount;
      vp = malloc(tmp_size);
      if (!vp) {
        rval = ON_AITUPDATE_E_GENERIC;
        goto bailout_finish;
      }
      ai->fBoundaries = static_cast<char**>(vp);
      for (int j = 0; j < in_ai->fBoundariesCount; j++) {
        ai->fBoundaries[j] = in_ai->fBoundaries[j];
      }
    }
    if (0 < in_ai->fNamesCount) {
      tmp_size = (sizeof(content::hbbtv::TDVBApplicationName) *
                  in_ai->fNamesCount);
      vp = malloc(tmp_size);
      if (vp) {
        rval = ON_AITUPDATE_E_GENERIC;
        goto bailout_finish;
      }
      ai->fNames = static_cast<content::hbbtv::TDVBApplicationName*>(vp);
      for (int j = 0; j < in_ai->fNamesCount; j += 1) {
        ai->fNames[j].fISO639Code[0] = in_ai->fNames[j].fISO639Code[0];
        ai->fNames[j].fISO639Code[1] = in_ai->fNames[j].fISO639Code[1];
        ai->fNames[j].fISO639Code[2] = in_ai->fNames[j].fISO639Code[2];
        ai->fNames[j].fName = in_ai->fNames[j].fName;
      }
    }
    ai->fIconLocator = in_ai->fIconLocator;
    ai->fIconFlags = in_ai->fIconFlags;
    switch (in_ai->fUsageType) {
    case OIPFAPPLICATION_USAGE_TYPE_GENERIC:
      ai->fUsageType = content::hbbtv::OIPFApplicationUsageType::OIPFAPPLICATION_USAGE_TYPE_GENERIC;
      break;
    case OIPFAPPLICATION_USAGE_TYPE_TELETEXT:
      ai->fUsageType = content::hbbtv::OIPFApplicationUsageType::OIPFAPPLICATION_USAGE_TYPE_TELETEXT;
      break;
    default:
      rval = ON_AITUPDATE_E_GENERIC;
      goto bailout_finish;
    }
    in_ai += 1;
    ai += 1;
  } while(ai < ai_endptr);
  rval = hbbtv_app_mgr_->OnAITUpdate(ai_array, in_count,
                                     in_isChannelChange, in_isXMLAIT);
bailout_finish:
  if (ai_array) {
    ai = ai_array;
    ai_endptr = ai + in_count;
    do {
      if (ai->fTransportProtocols) {
        free(static_cast<void*>(ai->fTransportProtocols));
        ai->fTransportProtocols = nullptr;
      }
      if (ai->fBoundaries) {
        free(static_cast<void*>(ai->fBoundaries));
        ai->fBoundaries = nullptr;
      }
      if (ai->fNames) {
        free(static_cast<void*>(ai->fNames));
        ai->fNames = nullptr;
      }
      ai += 1;
    } while (ai < ai_endptr);
    free(ai_array);
  }
  switch (rval) {
  case content::hbbtv::E_OK:
    return ON_AITUPDATE_E_OK;
  case content::hbbtv::E_NOTFOUND:
    return ON_AITUPDATE_E_NOTFOUND;
  }
#endif // defined(HBBTV_ON)
  return ON_AITUPDATE_E_GENERIC;
}

CefHBBTVAppMgrPtr CefHBBTVAppMgrImpl::GetHBBTVAppMgr() {
  return CefHBBTVAppMgrImpl::hbbtv_app_mgr_.get();
}

bool CefHBBTVAppMgrImpl::PreHandleKeyboardEvent(const CefKeyEvent& event,
                                                CefEventHandle os_event) {
#if defined(HBBTV_ON)
  if (!hbbtv_app_mgr_)
    return false;
  content::NativeWebKeyboardEvent native_event;

  // rudimentary translation which is enough for our testing purposes
  native_event.windowsKeyCode = event.windows_key_code;
  native_event.nativeKeyCode = event.native_key_code;
  native_event.isSystemKey = event.is_system_key ? 1 : 0;
  switch (event.type) {
  case KEYEVENT_RAWKEYDOWN:
  case KEYEVENT_KEYDOWN:
    native_event.type = blink::WebInputEvent::RawKeyDown;
    break;
  case KEYEVENT_KEYUP:
    native_event.type = blink::WebInputEvent::KeyUp;
    break;
  case KEYEVENT_CHAR:
    native_event.type = blink::WebInputEvent::Char;
    break;
  default:
    NOTREACHED();
    return false;
  }

  native_event.text[0] = event.character;
  native_event.unmodifiedText[0] = event.unmodified_character;

  native_event.setKeyIdentifierFromWindowsKeyCode();

  native_event.modifiers = 0;

  return hbbtv_app_mgr_->PreHandleKeyboardEvent(NULL, native_event);
#else
  return false;
#endif
}

#if defined(HBBTV_ON)
class HbbTVBrowserCefWindow : public content::hbbtv::HbbTVBrowserWindow {
 public:
  HbbTVBrowserCefWindow(CefBrowserHost* browser_host)
      : browser_host_(browser_host) {
  }

  void LoadURL(const GURL& url, bool focused) override {
    browser_host_->LoadURLNoFocus(url.spec(), focused);
  }

  content::WebContents* GetWebContents() override {
    return (content::WebContents*) browser_host_->GetWebContents();
  }

  void Close() override {
    browser_host_->CloseBrowser(false);
    delete this;
  }

 private:
  CefBrowserHost* browser_host_;
};

class HbbTVBrowserCefWindowFactory : public content::hbbtv::HbbTVBrowserWindowFactory {
 public:
  content::hbbtv::HbbTVBrowserWindow* CreateNewWindow(const GURL& url) const override {
    CefBrowserHost* browser_host = reinterpret_cast<CefBrowserHost*>(
        CefBrowserHost::CreateNewBrowserWindow(url.spec()));
    return new HbbTVBrowserCefWindow(browser_host);
  }
};
#endif

// CefHBBTVAppMgrImpl methods
CefHBBTVAppMgrImpl::CefHBBTVAppMgrImpl() {
#if defined(HBBTV_ON)
  if (!hbbtv_app_mgr_) {
    factory_.reset(new HbbTVBrowserCefWindowFactory());
    // Create the single application manager object.
    // The Browser Context is not needed as we're always using the first
    // see the hbbtv_apps_helper in browser_host_impl.
    hbbtv_app_mgr_.reset(new HBBTVAppMgrClassType(*factory_.get()));
  }
#endif
}

CefHBBTVAppMgrImpl::~CefHBBTVAppMgrImpl() {
}
