// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/common/shell_content_client.h"

#include "base/command_line.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "blink/public/resources/grit/blink_image_resources.h"
#include "build/build_config.h"
#include "content/app/resources/grit/content_resources.h"
#include "content/app/strings/grit/content_strings.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/user_agent.h"
#include "content/shell/common/shell_switches.h"
#include "grit/shell_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

#if defined(ENABLE_PLUGINS)
#if defined(HBBTV_ON)
namespace {

content::PepperPluginInfo::GetInterfaceFunc g_cbip_get_interface;
content::PepperPluginInfo::PPP_InitializeModuleFunc g_cbip_initialize_module;
content::PepperPluginInfo::PPP_ShutdownModuleFunc g_cbip_shutdown_module;

static const void* fn_oipf_video_get_interface(const char*) {
  return NULL;
}
static int fn_oipf_video_initialize_module(PP_Module, PPB_GetInterface) {
  // ./ppapi/api/pp_errors.idl
  return -2;  // PP_ERROR_FAILED
  // return 0;  // PP_OK
}
static void fn_oipf_video_shutdown_module() {
}
}  // namespace
#endif
#endif

namespace content {

namespace {

// This is the public key which the content shell will use to enable origin
// trial features.
// TODO(iclelland): Update this comment with the location of the public and
// private key files when the command-line tool CL lands
static const uint8_t kOriginTrialPublicKey[] = {
    0x75, 0x10, 0xac, 0xf9, 0x3a, 0x1c, 0xb8, 0xa9, 0x28, 0x70, 0xd2,
    0x9a, 0xd0, 0x0b, 0x59, 0xe1, 0xac, 0x2b, 0xb7, 0xd5, 0xca, 0x1f,
    0x64, 0x90, 0x08, 0x8e, 0xa8, 0xe0, 0x56, 0x3a, 0x04, 0xd0,
};
}  // namespace

std::string GetShellUserAgent() {
  std::string product = "Chrome/" CONTENT_SHELL_VERSION;
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch("user-agent"))
    return command_line->GetSwitchValueASCII("user-agent");
  if (command_line->HasSwitch(switches::kUseMobileUserAgent))
    product += " Mobile";
  return BuildUserAgentFromProduct(product);
}

ShellContentClient::ShellContentClient()
    : origin_trial_public_key_(base::StringPiece(
          reinterpret_cast<const char*>(kOriginTrialPublicKey),
          arraysize(kOriginTrialPublicKey))) {}

ShellContentClient::~ShellContentClient() {}

#if defined(ENABLE_PLUGINS)
void ShellContentClient::AddPepperPlugins(
    std::vector<content::PepperPluginInfo>* plugins) {
#if defined(HBBTV_ON)
    content::PepperPluginInfo cbip;
    cbip.is_internal = true;
    cbip.path =
        base::FilePath::FromUTF8Unsafe(cbip::kInternalCbipPluginFileName);
    cbip.name = cbip::kCbipPluginName;
    content::WebPluginMimeType cbip_js00_mime_type(
        cbip::kCbipJs00PluginMimeType,
        cbip::kCbipJs00PluginExtension,
        cbip::kCbipJs00PluginDescription);
    cbip.mime_types.push_back(cbip_js00_mime_type);

    content::WebPluginMimeType cbip_js01_mime_type(
        cbip::kCbipJs01PluginMimeType,
        cbip::kCbipJs01PluginExtension,
        cbip::kCbipJs01PluginDescription);
    cbip.mime_types.push_back(cbip_js01_mime_type);

    content::WebPluginMimeType cbip_oipf_appmgr_mime_type(
        cbip::kCbipOipfApplicationManagerPluginMimeType,
        cbip::kCbipOipfApplicationManagerPluginExtension,
        cbip::kCbipOipfApplicationManagerPluginDescription);
    cbip.mime_types.push_back(cbip_oipf_appmgr_mime_type);

    content::WebPluginMimeType cbip_oipf_video_broadcast_mime_type(
        cbip::kCbipOipfVideoBoradcastPluginMimeType,
        cbip::kCbipOipfVideoBoradcastPluginExtension,
        cbip::kCbipOipfVideoBoradcastPluginDescription);
    cbip.mime_types.push_back(cbip_oipf_video_broadcast_mime_type);

    content::WebPluginMimeType oipf_configuration_mime_type(
        cbip::kOipfConfigurationPluginMimeType,
        cbip::kOipfConfigurationPluginExtension,
        cbip::kOipfConfigurationPluginDescription);
    cbip.mime_types.push_back(oipf_configuration_mime_type);

    content::WebPluginMimeType oipf_parental_control_manager_mime_type(
        cbip::kOipfParentalControlManagerPluginMimeType,
        cbip::kOipfParentalControlManagerPluginExtension,
        cbip::kOipfParentalControlManagerPluginDescription);
    cbip.mime_types.push_back(oipf_parental_control_manager_mime_type);

    content::WebPluginMimeType oipf_capabilities_mime_type(
        cbip::kOipfCapabilitiesPluginMimeType,
        cbip::kOipfCapabilitiesPluginExtension,
        cbip::kOipfCapabilitiesPluginDescription);
    cbip.mime_types.push_back(oipf_capabilities_mime_type);

    cbip.internal_entry_points.get_interface = g_cbip_get_interface;
    cbip.internal_entry_points.initialize_module = g_cbip_initialize_module;
    cbip.internal_entry_points.shutdown_module = g_cbip_shutdown_module;
    cbip.permissions = ppapi::PERMISSION_PRIVATE | ppapi::PERMISSION_DEV;
    plugins->push_back(cbip);
  {
    content::PepperPluginInfo pinfo;
    pinfo.is_internal = true;
    pinfo.path =
        base::FilePath::FromUTF8Unsafe("/noexist/libdummyoipfvideo.so");
    pinfo.name = "OIPF VIDEO";
    {
      content::WebPluginMimeType mime_type("application/x-oipf-video",
                                           "",
                                           "oipf video plugin");
      pinfo.mime_types.push_back(mime_type);
    }
    {
      content::WebPluginMimeType mime_type("video/mp4",
                                           "",
                                           "oipf video plugin");
      pinfo.mime_types.push_back(mime_type);
    }
    {
      content::WebPluginMimeType mime_type("audio/mp4",
                                           "",
                                           "oipf video plugin");
      pinfo.mime_types.push_back(mime_type);
    }
    {
      content::WebPluginMimeType mime_type("audio/mpeg",
                                           "",
                                           "oipf video plugin");
      pinfo.mime_types.push_back(mime_type);
    }
    pinfo.internal_entry_points.get_interface = fn_oipf_video_get_interface;
    pinfo.internal_entry_points.initialize_module =
        fn_oipf_video_initialize_module;
    pinfo.internal_entry_points.shutdown_module = fn_oipf_video_shutdown_module;
    pinfo.permissions = ppapi::PERMISSION_PRIVATE | ppapi::PERMISSION_DEV;
    plugins->push_back(pinfo);
  }
#endif
}

#if defined(HBBTV_ON)
void ShellContentClient::SetCBIPEntryFunctions(
    content::PepperPluginInfo::GetInterfaceFunc get_interface,
    content::PepperPluginInfo::PPP_InitializeModuleFunc initialize_module,
    content::PepperPluginInfo::PPP_ShutdownModuleFunc shutdown_module) {
  g_cbip_get_interface = get_interface;
  g_cbip_initialize_module = initialize_module;
  g_cbip_shutdown_module = shutdown_module;
}
#endif
#endif  // ENABLE_PLUGINS

std::string ShellContentClient::GetUserAgent() const {
  return GetShellUserAgent();
}

base::string16 ShellContentClient::GetLocalizedString(int message_id) const {
  if (switches::IsRunLayoutTestSwitchPresent()) {
    switch (message_id) {
      case IDS_FORM_OTHER_DATE_LABEL:
        return base::ASCIIToUTF16("<<OtherDateLabel>>");
      case IDS_FORM_OTHER_MONTH_LABEL:
        return base::ASCIIToUTF16("<<OtherMonthLabel>>");
      case IDS_FORM_OTHER_TIME_LABEL:
        return base::ASCIIToUTF16("<<OtherTimeLabel>>");
      case IDS_FORM_OTHER_WEEK_LABEL:
        return base::ASCIIToUTF16("<<OtherWeekLabel>>");
      case IDS_FORM_CALENDAR_CLEAR:
        return base::ASCIIToUTF16("<<CalendarClear>>");
      case IDS_FORM_CALENDAR_TODAY:
        return base::ASCIIToUTF16("<<CalendarToday>>");
      case IDS_FORM_THIS_MONTH_LABEL:
        return base::ASCIIToUTF16("<<ThisMonthLabel>>");
      case IDS_FORM_THIS_WEEK_LABEL:
        return base::ASCIIToUTF16("<<ThisWeekLabel>>");
    }
  }
  return l10n_util::GetStringUTF16(message_id);
}

base::StringPiece ShellContentClient::GetDataResource(
    int resource_id,
    ui::ScaleFactor scale_factor) const {
  if (switches::IsRunLayoutTestSwitchPresent()) {
    switch (resource_id) {
      case IDR_BROKENIMAGE:
#if defined(OS_MACOSX)
        resource_id = IDR_CONTENT_SHELL_MISSING_IMAGE_PNG;
#else
        resource_id = IDR_CONTENT_SHELL_MISSING_IMAGE_GIF;
#endif
        break;

      case IDR_TEXTAREA_RESIZER:
        resource_id = IDR_CONTENT_SHELL_TEXT_AREA_RESIZE_CORNER_PNG;
        break;
    }
  }
  return ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

base::RefCountedStaticMemory* ShellContentClient::GetDataResourceBytes(
    int resource_id) const {
  return ResourceBundle::GetSharedInstance().LoadDataResourceBytes(resource_id);
}

gfx::Image& ShellContentClient::GetNativeImageNamed(int resource_id) const {
  return ResourceBundle::GetSharedInstance().GetNativeImageNamed(resource_id);
}

bool ShellContentClient::IsSupplementarySiteIsolationModeEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kIsolateSitesForTesting);
}

base::StringPiece ShellContentClient::GetOriginTrialPublicKey() {
  return origin_trial_public_key_;
}

}  // namespace content
