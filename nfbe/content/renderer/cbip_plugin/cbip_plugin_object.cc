// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_plugin_object.h"

#include "base/id_map.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "content/renderer/cbip_plugin/cbip_constants.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object_js00.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object_js01.h"
#if defined(HBBTV_ON)
#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_application_manager.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_capabilities.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_configuration_plugin.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_parental_control_manager.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_video_broadcast.h"
#endif

#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/plugin_module.h"
#include "gin/object_template_builder.h"
#include "v8/include/v8.h"

#define X_LOG_V 0

using ppapi::ScopedPPVar;

namespace content {

namespace {
typedef IDMap<CbipPluginObject> CbipInstanceMap;
base::LazyInstance<CbipInstanceMap>::Leaky
    g_all_cbip_instances_map = LAZY_INSTANCE_INITIALIZER;
}  // namespace

// static
CbipPluginObject* CbipPluginObject::FromInstanceId(int id) {
    return g_all_cbip_instances_map.Pointer()->Lookup(id);
}

CbipPluginObject::CbipPluginObject()
    : cbip_instance_id_(
        g_all_cbip_instances_map.Pointer()->Add(this)) {
}

CbipPluginObject::~CbipPluginObject() {
    g_all_cbip_instances_map.Pointer()->Remove(cbip_instance_id_);
}

// static
CbipPluginObject* CbipPluginObject::FromV8Object(
    v8::Isolate* isolate,
    v8::Handle<v8::Object> v8_object) {
  CbipPluginObjectJs00* cbip_plugin_object_js00 =
      CbipPluginObjectJs00::FromV8Object(isolate, v8_object);
  if (cbip_plugin_object_js00)
    return cbip_plugin_object_js00;

  CbipPluginObjectJs01* cbip_plugin_object_js01 =
      CbipPluginObjectJs01::FromV8Object(isolate, v8_object);
  if (cbip_plugin_object_js01)
    return cbip_plugin_object_js01;

#if defined(HBBTV_ON)
  {
    CbipPluginObjectOipfApplicationManager* o =
        CbipPluginObjectOipfApplicationManager::FromV8Object(isolate, v8_object);
    if (o)
        return o;

    CbipPluginObjectOipfVideoBroadcast* b =
      CbipPluginObjectOipfVideoBroadcast::FromV8Object(isolate, v8_object);
    if (b)
      return b;

    ConfigurationPlugin* pConfigPlugin =
        ConfigurationPlugin::FromV8Object(isolate, v8_object);
    if (pConfigPlugin)
      return pConfigPlugin;

    CbipPluginObjectParentalControlManager* pParentalControlManager =
        CbipPluginObjectParentalControlManager::FromV8Object(isolate, v8_object);
    if (pParentalControlManager)
      return pParentalControlManager;

  CbipPluginObjectCapabilities* pCapabilitiesPlugin =
      CbipPluginObjectCapabilities::FromV8Object(isolate, v8_object);
    if (pCapabilitiesPlugin)
      return pCapabilitiesPlugin;
}
#endif

  return NULL;
}

// static
PP_Var CbipPluginObject::Create(PepperPluginInstanceImpl* instance,
    const std::string& mime_type) {
  {
    VLOG(X_LOG_V) << "ZZZZ: CbipPluginObject::Create:"
                  << " mime_type = " << mime_type;
  }

  if (mime_type == "application/x-cbip-js00")
    return CbipPluginObjectJs00::Create(instance);

  if (mime_type == "application/x-cbip-js01")
    return CbipPluginObjectJs01::Create(instance);

#if defined(HBBTV_ON)
  /* FIXME: find who translates, and choose proper comparison method. */
  if (mime_type == "application/oipfapplicationmanager")
    return CbipPluginObjectOipfApplicationManager::Create(instance);

  if (mime_type == "video/broadcast")
    return CbipPluginObjectOipfVideoBroadcast::Create(instance);

  if (mime_type == "application/oipfconfiguration" ||
      mime_type == "application/oipfConfiguration")
    return ConfigurationPlugin::Create(instance);

  if (mime_type == "application/oipfparentalcontrolmanager" ||
      mime_type ==  "application/oipfParentalControlManager")
    return CbipPluginObjectParentalControlManager::Create(instance);

  if (mime_type == "application/oipfCapabilities" ||
      mime_type == "application/oipfcapabilities")
    return CbipPluginObjectCapabilities::Create(instance);
#endif

  return PP_MakeUndefined();
}

bool CbipPluginObject::OnAppMgrMessageReceived(const IPC::Message& msg) {
  return false;
}

}  // namespace content
