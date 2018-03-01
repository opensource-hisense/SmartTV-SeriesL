// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_capabilities.h"

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
#include "v8/include/v8.h"

#include "ipc/ipc_message.h"

using ppapi::ScopedPPVar;
namespace content {
namespace {
const char kPropertyXmlCapabilities[] = "xmlCapabilities";
const char kPropertyExtraHDVideoDecodes[] = "extraHDVideoDecodes";
const char kPropertyExtraSDVideoDecodes[] = "extraSDVideoDecodes";
const char kMethodHasCapability[] = "hasCapability";
const char kMethodToString[] = "toString";

CbipPluginDispatcher* dispatcher() {
  if (RenderThreadImpl::current())
    return RenderThreadImpl::current()->cbip_plugin_dispatcher();
  return nullptr;
}
}  // namespace

CbipPluginObjectCapabilities::CbipPluginObjectCapabilities(
    PepperPluginInstanceImpl* instance)
    : gin::NamedPropertyInterceptor(instance->GetIsolate(), this),
      instance_(instance),
      weak_ptr_factory_(this) {
  if (dispatcher()) {
    ipc_ = dispatcher()->GetAppMgr(cbip_instance_id_);
    const int routing_id = instance_->render_frame()->GetRoutingID();
    ipc_->BindOipfCapabilities(routing_id);
  } else {
    ipc_ = nullptr;
  }
  if (instance_)
    instance_->AddCbipPluginObject(this);
}

CbipPluginObjectCapabilities::~CbipPluginObjectCapabilities() {
  LOG(INFO) << "CbipPluginObjectCapabilities::~CbipPluginObjectCapabilities";
  if (instance_) {
    instance_->RemoveCbipPluginObject(this);
  }
  if (dispatcher())
    dispatcher()->ReleaseAppMgr(cbip_instance_id_, ipc_.get());
}

// static
gin::WrapperInfo CbipPluginObjectCapabilities::kWrapperInfo =
    {gin::kEmbedderNativeGin};

// static
CbipPluginObjectCapabilities* CbipPluginObjectCapabilities::FromV8Object(
    v8::Isolate* isolate, v8::Handle<v8::Object> v8_object) {
  CbipPluginObjectCapabilities* capabilities_plugin_object;
  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate,
                         v8_object,
                         &capabilities_plugin_object)) {
    return capabilities_plugin_object;
  }
  return nullptr;
}

// static
PP_Var CbipPluginObjectCapabilities::
    Create(PepperPluginInstanceImpl* instance) {
  V8VarConverter var_converter(instance->pp_instance(),
                               V8VarConverter::kAllowObjectVars);
  PepperTryCatchVar try_catch(instance, &var_converter, NULL);

  CbipPluginObjectCapabilities* capability_plugin_object =
      new CbipPluginObjectCapabilities(instance);
  gin::Handle<CbipPluginObjectCapabilities> object =
      gin::CreateHandle(instance->GetIsolate(), capability_plugin_object);
  ScopedPPVar result = try_catch.FromV8(object.ToV8());
  DCHECK(!try_catch.HasException());

  return result.Release();
}

v8::Local<v8::Value> CbipPluginObjectCapabilities::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  LOG(INFO) << "CbipPluginObjectCapabilities::GetNamedProperty:"
            << " " << identifier;

  if (!instance_)
    return v8::Local<v8::Value>();

  auto find = is_property_set_.find(identifier);
  if (find != is_property_set_.end())
    return v8::Local<v8::Value>();

  const int routing_id = instance_->render_frame()->GetRoutingID();

  if (identifier == kPropertyXmlCapabilities) {
    std::string xml_capabilities;
    if (ipc())
      ipc()->GetXmlCapabilities(routing_id, &xml_capabilities);
    v8::Local<v8::Object> object = instance_->render_frame()->
        CreateAndGetDocumentV8Object(xml_capabilities);
    return v8::Local<v8::Object>::New(isolate, object);
  }
  if (identifier == kPropertyExtraSDVideoDecodes) {
    int32_t extra_sd_video_decodes = 0;
    if (ipc())
      ipc()->GetExtraSDVideoDecodes(routing_id, &extra_sd_video_decodes);
    return v8::Integer::New(isolate, extra_sd_video_decodes);
  }
  if (identifier == kPropertyExtraHDVideoDecodes) {
    int32_t extra_hd_video_decodes = 0;
    if (ipc())
      ipc()->GetExtraSDVideoDecodes(routing_id, &extra_hd_video_decodes);
    return v8::Integer::New(isolate, extra_hd_video_decodes);
  }
  if (identifier == kMethodHasCapability) {
    return gin::CreateFunctionTemplate(
        isolate,
        base::Bind(&CbipPluginObjectCapabilities::hasCapability,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kMethodToString) {
    return gin::CreateFunctionTemplate(
        isolate,
        base::Bind(&CbipPluginObjectCapabilities::toString,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  return v8::Local<v8::Value>();
}

bool CbipPluginObjectCapabilities::SetNamedProperty(v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (identifier == kPropertyXmlCapabilities ||
      identifier == kPropertyExtraSDVideoDecodes ||
      identifier == kPropertyExtraHDVideoDecodes)
    // all properties are readonly so no implementation needed.
    return true;

  // Contents can set value to methods, so it act as normal case.
  is_property_set_.insert(identifier);
  return false;
}

std::vector<std::string> CbipPluginObjectCapabilities::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kPropertyXmlCapabilities);
  result.push_back(kPropertyExtraSDVideoDecodes);
  result.push_back(kPropertyExtraHDVideoDecodes);
  result.push_back(kMethodHasCapability);
  // kMethodToString isn't enumerable.
  return result;
}

void CbipPluginObjectCapabilities::hasCapability(gin::Arguments* args) {
  v8::Handle<v8::Value> js_profile_name;
  if (!args->GetNext(&js_profile_name)) {
    LOG(INFO) << "CbipPluginObjectCapabilities::hasCapability "
              << "no arguments passed";
    return args->Return(
               static_cast<v8::Local<v8::Value>>(v8::False(args->isolate())));
  }
  if (!js_profile_name->IsString()) {
    LOG(INFO) << "CbipPluginObjectCapabilities::hasCapability "
              << "invalid argument passed";
    return args->Return(
               static_cast<v8::Local<v8::Value>>(v8::False(args->isolate())));
  }
  if (!ipc())
    return args->Return(
               static_cast<v8::Local<v8::Value>>(v8::False(args->isolate())));

  v8::String::Utf8Value js_string(js_profile_name->ToString());
  std::string profile_name(*js_string);
  const int routing_id = instance_->render_frame()->GetRoutingID();
  bool has_capability = ipc()->HasCapabilities(routing_id, profile_name);
  args->Return(static_cast<v8::Local<v8::Value>>(
      v8::Boolean::New(args->isolate(), has_capability)));
}

void CbipPluginObjectCapabilities::toString(gin::Arguments* args) {
  std::string class_name("[object OipfCapabilities]");
  v8::Handle<v8::Value> result =
      v8::String::NewFromUtf8(args->isolate(), class_name.c_str(),
                              v8::String::kNormalString,
                              class_name.length());
  args->Return(result);
}

void CbipPluginObjectCapabilities::InstanceDeleted() {
  DCHECK(instance_);
  const int routing_id = instance_->render_frame()->GetRoutingID();
  if (ipc_)
    ipc_->UnbindOipfCapabilities(routing_id);
  instance_ = nullptr;
}

gin::ObjectTemplateBuilder CbipPluginObjectCapabilities::
    GetObjectTemplateBuilder(v8::Isolate* isolate) {
  return Wrappable<CbipPluginObjectCapabilities>::
         GetObjectTemplateBuilder(isolate).AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor* CbipPluginObjectCapabilities::
    asGinNamedPropertyInterceptor() {
  return this;
}

v8::Isolate* CbipPluginObjectCapabilities::GetIsolate() {
  if (instance_)
    return instance_->GetIsolate();
  return nullptr;
}

}  // namespace content
