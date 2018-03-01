// Copyright (c) 2016 ACCESS CO. LTD. All rights reserved.

#ifndef CEF_LIBCEF_COMMON_HBBTV_APP_MGR_IMPL_H_
#define CEF_LIBCEF_COMMON_HBBTV_APP_MGR_IMPL_H_
#pragma once

#include "include/cef_hbbtv_app_mgr.h"

#if defined(HBBTV_ON)
namespace content {
namespace hbbtv {
class HBBTVAppMgr;
class HbbTVBrowserWindowFactory;
}
}

using HBBTVAppMgrClassType = content::hbbtv::HBBTVAppMgr;
using HbbTVBrowserWindowFactoryClassType = content::hbbtv::HbbTVBrowserWindowFactory;
#else
// This is only to avoid creating separate CEF wrappers for
// the case where HBBTV is disabled.
using HBBTVAppMgrClassType = void*;
using HbbTVBrowserWindowFactoryClassType = void*;
#endif

// CefHBBTVAppMgr implementation
class CefHBBTVAppMgrImpl: public CefHBBTVAppMgr {
 public:
  CefHBBTVAppMgrImpl();
  ~CefHBBTVAppMgrImpl();

  // CefHBBTVAppMgr methods.
  cef_hbbtv_on_aitupdate_return_value_t OnAITUpdate(
      cef_hbbtv_dvb_application_information_t* in_ai, int in_count,
      bool in_isChannelChange, bool in_isXMLAIT) override;

  CefHBBTVAppMgrPtr GetHBBTVAppMgr() override;
  bool PreHandleKeyboardEvent(const CefKeyEvent& event,
                              CefEventHandle os_event) override;

 private:
  static std::unique_ptr<HBBTVAppMgrClassType> hbbtv_app_mgr_;
  static std::unique_ptr<HbbTVBrowserWindowFactoryClassType> factory_;

  // Include the default reference counting implementation.
  IMPLEMENT_REFCOUNTING(CefHBBTVAppMgrImpl);
};

#endif  // CEF_LIBCEF_COMMON_HBBTV_APP_MGR_IMPL_H_
