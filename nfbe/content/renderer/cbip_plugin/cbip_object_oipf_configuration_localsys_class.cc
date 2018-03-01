// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_object_oipf_configuration_localsys_class.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_configuration_plugin.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/cbip/hbbtv_messages.h"
#include "content/renderer/cbip_plugin/cbip_plugin_dispatcher.h"
#include "content/renderer/pepper/message_channel.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/pepper_try_catch.h"
#include "content/renderer/pepper/plugin_module.h"
#include "content/renderer/pepper/v8_var_converter.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "gin/public/gin_embedders.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"
#include "v8/include/v8.h"

#define X_LOG_V 0

const char kDeviceId[] = "deviceID";
const char kModelName[] = "modelName";
const char kVendorName[] = "vendorName";
const char kSoftwareVersion[] = "softwareVersion";
const char kHardwareVersion[] = "hardwareVersion";
const char kSerialNumber[] = "serialNumber";

namespace content {
LocalSystemClass::LocalSystemClass(ConfigurationPlugin* owner,
    ConfigurationLocalSysParam* configuration_local_sys_param)
    : gin::NamedPropertyInterceptor(owner->GetIsolate(), this),
      owner_(owner),
      weak_ptr_factory_(this) {
  if (configuration_local_sys_param) {
    deviceId = configuration_local_sys_param->deviceId;
    modelName = configuration_local_sys_param->modelName;
    vendorName = configuration_local_sys_param->vendorName;
    softwareVersion = configuration_local_sys_param->softwareVersion;
    hardwareVersion = configuration_local_sys_param->hardwareVersion;
    serialNumber = configuration_local_sys_param->serialNumber;
  }
}

LocalSystemClass::~LocalSystemClass() {
  VLOG(X_LOG_V) << "ZZZZ LocalSystemClass::~LocalSystemClass:";
}

// static
gin::WrapperInfo LocalSystemClass::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
LocalSystemClass* LocalSystemClass::FromV8Object(
    v8::Isolate* isolate, v8::Handle<v8::Object> v8_object) {
  LocalSystemClass* localSystem_object;
  if (!v8_object.IsEmpty() &&
    gin::ConvertFromV8(isolate, v8_object, &localSystem_object)) {
    return localSystem_object;
  }
  return NULL;
}

v8::Local<v8::Value> LocalSystemClass::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  VLOG(X_LOG_V) << "ZZZZ LocalSystemClass::GetNamedProperty:" << identifier;

  if (!owner_)
    return v8::Local<v8::Value>();
  if (!GetIsolate())
    return v8::Local<v8::Value>();

  DCHECK(isolate == owner_->GetIsolate());

  std::string result;
  if (identifier == kDeviceId) {
    result = GetProperty_deviceId();
  }
  if (identifier == kModelName) {
    result = GetProperty_modelName();
  }
  if (identifier == kVendorName) {
    result = GetProperty_vendorName();
  }
  if (identifier == kSoftwareVersion) {
    result = GetProperty_softwareVersion();
  }
  if (identifier == kHardwareVersion) {
    result = GetProperty_hardwareVersion();
  }
  if (identifier == kSerialNumber) {
    result = GetProperty_serialNumber();
  }
  return ResultStringToV8(isolate, result);
}

std::vector<std::string> LocalSystemClass::EnumerateNamedProperties(
        v8::Isolate* isolate) {
    std::vector<std::string> result;
    result.push_back(kDeviceId);
    result.push_back(kModelName);
    result.push_back(kVendorName);
    result.push_back(kSoftwareVersion);
    result.push_back(kHardwareVersion);
    result.push_back(kSerialNumber);
    return result;
}

v8::Isolate* LocalSystemClass::GetIsolate() {
    if (owner_)
        return owner_->GetIsolate();
    return 0;
}

void LocalSystemClass::OwnerPluginDeleted() {
    owner_ = 0;
}

gin::ObjectTemplateBuilder LocalSystemClass::GetObjectTemplateBuilder(
        v8::Isolate* isolate) {
    return Wrappable<LocalSystemClass>::GetObjectTemplateBuilder(isolate)
            .AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor*
    LocalSystemClass::asGinNamedPropertyInterceptor() {
  return this;
}

v8::Local<v8::Value> LocalSystemClass::ResultStringToV8(v8::Isolate* isolate,
                                                        std::string result) {
  VLOG(X_LOG_V) << "ZZZZ LocalSystemClass::ResultStringToV8:" << result;
  if (result.empty())
    return v8::Local<v8::Value>();
  v8::Local<v8::String> str  = gin::StringToV8(isolate, result);
  v8::Local<v8::Value> value = v8::Local<v8::String>::New(isolate, str);

  return value;
}

std::string LocalSystemClass::GetProperty_deviceId() {
  return deviceId;
}

std::string LocalSystemClass::GetProperty_modelName() {
  return modelName;
}

std::string LocalSystemClass::GetProperty_vendorName() {
  return vendorName;
}

std::string LocalSystemClass::GetProperty_softwareVersion() {
  return softwareVersion;
}

std::string LocalSystemClass::GetProperty_hardwareVersion() {
  return hardwareVersion;
}

std::string LocalSystemClass::GetProperty_serialNumber() {
  return serialNumber;
}

}  // end namespace content
