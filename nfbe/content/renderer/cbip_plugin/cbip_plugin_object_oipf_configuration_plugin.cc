// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_configuration_plugin.h"
#include "access/content/common/cbip/oipf_common_structs.h"
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

#include "ipc/ipc_message.h"

#define X_LOG_V 0

using ppapi::ScopedPPVar;

const char kConfiguration[] = "configuration";
const char kLocalSystem[] = "localSystem";
const char kMethodToString[] = "toString";

namespace content {

struct ConfigurationParam;

CbipPluginDispatcher* dispatcher() {
  return RenderThreadImpl::current()->cbip_plugin_dispatcher();
}

ConfigurationPlugin::ConfigurationPlugin(PepperPluginInstanceImpl* instance)
    : gin::NamedPropertyInterceptor(instance->GetIsolate(), this),
      instance_(instance),
      ipc_(dispatcher()->GetAppMgr(cbip_instance_id_)),
      weak_ptr_factory_(this) {
  instance_->AddCbipPluginObject(this);
  int routing_id = instance_->render_frame()->GetRoutingID();
  v8::Isolate* isolate = instance_->GetIsolate();

  scoped_ptr<ConfigurationParam> configuration_param;
  configuration_param.reset(new ConfigurationParam());
  ipc()->GetConfigurationData(routing_id, configuration_param.get());

  ConfigurationClass* configClass =
      new ConfigurationClass(this, configuration_param.get());
  configurationObj.Reset(isolate, configClass->GetWrapper(isolate));

  scoped_ptr<ConfigurationLocalSysParam> configuration_local_sys_param;
  configuration_local_sys_param.reset(new ConfigurationLocalSysParam());
  ipc()->GetConfigurationLocalSystem(routing_id,
                                     configuration_local_sys_param.get());

  LocalSystemClass* localSys =
      new LocalSystemClass(this, configuration_local_sys_param.get());
  localSystemObj.Reset(isolate, localSys->GetWrapper(isolate));
}

ConfigurationPlugin::~ConfigurationPlugin() {
  VLOG(X_LOG_V) << "ZZZZ ConfigurationPlugin::~ConfigurationPlugin";
  if (instance_) {
    instance_->RemoveCbipPluginObject(this);

    v8::Isolate* isolate = instance_->GetIsolate();
    ConfigurationClass* pConfigurationClass =
        ConfigurationClass::FromV8Object(isolate,
        v8::Local<v8::Object>::New(isolate, configurationObj));
    DCHECK(pConfigurationClass);
    pConfigurationClass->OwnerPluginDeleted();

    LocalSystemClass* pLocalSystemClass =
        LocalSystemClass::FromV8Object(isolate,
        v8::Local<v8::Object>::New(isolate, localSystemObj));
    DCHECK(pLocalSystemClass);
    pLocalSystemClass->OwnerPluginDeleted();
  }
  if (dispatcher())
    dispatcher()->ReleaseAppMgr(cbip_instance_id_, ipc_.get());
}

// static
gin::WrapperInfo ConfigurationPlugin::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
ConfigurationPlugin* ConfigurationPlugin::FromV8Object(
    v8::Isolate* isolate, v8::Handle<v8::Object> v8_object) {
  ConfigurationPlugin* configuration_plugin_object;
  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate,
                         v8_object,
                         &configuration_plugin_object)) {
    return configuration_plugin_object;
  }

return NULL;
}

// static
PP_Var ConfigurationPlugin::Create(PepperPluginInstanceImpl* instance) {
  V8VarConverter var_converter(instance->pp_instance(),
  V8VarConverter::kAllowObjectVars);
  PepperTryCatchVar try_catch(instance, &var_converter, NULL);

  ConfigurationPlugin* configuration_plugin_object =
      new ConfigurationPlugin(instance);
  gin::Handle<ConfigurationPlugin> object =
    gin::CreateHandle(instance->GetIsolate(), configuration_plugin_object);
  ScopedPPVar result = try_catch.FromV8(object.ToV8());
  DCHECK(!try_catch.HasException());

  return result.Release();
}

v8::Local<v8::Value> ConfigurationPlugin::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  VLOG(X_LOG_V) << "ZZZZ ConfigurationPlugin::GetNamedProperty:"
                << " " << identifier;

  if (!instance_)
    return v8::Local<v8::Value>();

  if (identifier == kConfiguration)
    return v8::Local<v8::Object>::New(isolate, configurationObj);

  if (identifier == kLocalSystem)
    return v8::Local<v8::Object>::New(isolate, localSystemObj);

  if (identifier == kMethodToString) {
    return gin::CreateFunctionTemplate(
        isolate,
        base::Bind(&ConfigurationPlugin::toString,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  return v8::Local<v8::Value>();
}

bool ConfigurationPlugin::SetNamedProperty(v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  return false;
}

std::vector<std::string> ConfigurationPlugin::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kConfiguration);
  // kMethodToString isn't enumerable.
  return result;
}

void ConfigurationPlugin::toString(gin::Arguments* args) {
  std::string class_name("[object ConfigurationPlugin]");
  v8::Handle<v8::Value> result =
      v8::String::NewFromUtf8(args->isolate(), class_name.c_str(),
                              v8::String::kNormalString,
                              class_name.length());
  args->Return(result);
}

void ConfigurationPlugin::InstanceDeleted() {
  instance_ = NULL;
}

gin::ObjectTemplateBuilder ConfigurationPlugin::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<ConfigurationPlugin>::GetObjectTemplateBuilder(isolate)
      .AddNamedPropertyInterceptor();
  }

  gin::NamedPropertyInterceptor*
  ConfigurationPlugin::asGinNamedPropertyInterceptor() {
  return this;
}

v8::Isolate* ConfigurationPlugin::GetIsolate() {
  if (instance_)
    return instance_->GetIsolate();
  return nullptr;
}

}  // namespace content
