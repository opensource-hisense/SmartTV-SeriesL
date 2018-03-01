// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_configuration_class.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_configuration_plugin.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/cbip/hbbtv_messages.h"
#include "content/renderer/cbip_plugin/cbip_plugin_dispatcher.h"
#include "content/renderer/cbip_plugin/cbip_plugin_ipc.h"
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

const char kPreferredAudioLanguage[] = "preferredAudioLanguage";
const char kPreferredSubtitleLanguage[] = "preferredSubtitleLanguage";
const char kPreferredUILanguage[] = "preferredUILanguage";
const char kCountryId[] = "countryId";
const char kToString[] = "toString";
const char kSubtitlesEnabled[] ="subtitlesEnabled";
const char kTimeShiftSynchronized[] = "timeShiftSynchronized";

namespace content {

ConfigurationClass::ConfigurationClass(ConfigurationPlugin* owner,
                                       ConfigurationParam *configuration_param)
    : gin::NamedPropertyInterceptor(owner->GetIsolate(), this),
      owner_(owner),
      weak_ptr_factory_(this) {
  if (configuration_param) {
    preferredAudioLanguage = configuration_param->preferredAudioLanguage;
    preferredSubtitleLanguage = configuration_param->preferredSubtitleLanguage;
    preferredUILanguage = configuration_param->preferredUILanguage;
    countryId = configuration_param->countryId;
    subtitles_enabled_ = configuration_param->subtitles_enabled;
    time_shift_synchronized_ = configuration_param->time_shift_synchronized;
  }
}

ConfigurationClass::~ConfigurationClass() {
  VLOG(X_LOG_V) << "ConfigurationClass::~ConfigurationClass:";
}

// static
gin::WrapperInfo ConfigurationClass::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
ConfigurationClass* ConfigurationClass::FromV8Object(
    v8::Isolate* isolate, v8::Handle<v8::Object> v8_object) {
  ConfigurationClass* configuration_object;
  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &configuration_object)) {
    return configuration_object;
  }
  return NULL;
}

v8::Local<v8::Value> ConfigurationClass::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  VLOG(X_LOG_V) << "ConfigurationClass::GetNamedProperty:" << identifier;

  if (!owner_)
    return v8::Local<v8::Value>();
  if (!GetIsolate())
    return v8::Local<v8::Value>();

  DCHECK(isolate == owner_->GetIsolate());

  std::string result;
  if (identifier == kPreferredAudioLanguage) {
    result = GetProperty_preferredAudioLanguage();
  }
  if (identifier == kPreferredSubtitleLanguage) {
    result = GetProperty_preferredSubtitleLanguage();
  }
  if (identifier == kPreferredUILanguage) {
    result = GetProperty_preferredUILanguage();
  }
  if (identifier == kCountryId) {
    result = GetProperty_countryId();
  }
  if (identifier == kSubtitlesEnabled) {
    return v8::Boolean::New(isolate, subtitles_enabled_);
  }
  if (identifier == kTimeShiftSynchronized) {
    return v8::Boolean::New(isolate, time_shift_synchronized_);
  }
  if (identifier == kToString) {
    return gin::CreateFunctionTemplate(
        isolate, base::Bind(&ConfigurationClass::toString,
                            weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  return ResultStringToV8(isolate, result);
}

std::vector<std::string> ConfigurationClass::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kPreferredUILanguage);
  result.push_back(kPreferredAudioLanguage);
  result.push_back(kPreferredSubtitleLanguage);
  result.push_back(kCountryId);
  result.push_back(kSubtitlesEnabled);
  result.push_back(kTimeShiftSynchronized);
  // kToString isn't enumerable.
  return result;
}

v8::Isolate* ConfigurationClass::GetIsolate() {
  if (owner_)
    return owner_->GetIsolate();
  return 0;
}

void ConfigurationClass::OwnerPluginDeleted() {
  owner_ = 0;
}

gin::ObjectTemplateBuilder ConfigurationClass::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<ConfigurationClass>::GetObjectTemplateBuilder(isolate)
         .AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor*
    ConfigurationClass::asGinNamedPropertyInterceptor() {
  return this;
}

void ConfigurationClass::toString(gin::Arguments* args) {
  const std::string class_name("[object Configuration]");
  v8::Handle<v8::Value> result = v8::String::NewFromUtf8(
      args->isolate(), class_name.c_str(), v8::String::kNormalString,
      class_name.length());
  args->Return(result);
}

v8::Local<v8::Value> ConfigurationClass::ResultStringToV8(v8::Isolate* isolate,
                                                          std::string result) {
  VLOG(X_LOG_V) << "ConfigurationClass::ResultStringToV8:"<< " " << result;

  if (result.empty())
    return v8::Local<v8::Value>();

  v8::Local<v8::String> str  = gin::StringToV8(isolate, result);
  v8::Local<v8::Value> value = v8::Local<v8::String>::New(isolate, str);

  return value;
}

std::string ConfigurationClass::GetProperty_preferredUILanguage() {
  return preferredUILanguage;
}

std::string ConfigurationClass::GetProperty_countryId() {
  return countryId;
}

std::string ConfigurationClass::GetProperty_preferredAudioLanguage() {
  return preferredAudioLanguage;
}

std::string ConfigurationClass::GetProperty_preferredSubtitleLanguage() {
  return preferredSubtitleLanguage;
}

}  // end namespace content
