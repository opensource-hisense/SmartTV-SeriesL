// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_parental_control_manager.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/cbip/hbbtv_messages.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_parental_rating_scheme_collection_class.h"
#include "content/renderer/cbip_plugin/cbip_plugin_dispatcher.h"
#include "content/renderer/pepper/message_channel.h"
#include "content/renderer/pepper/pepper_try_catch.h"
#include "content/renderer/pepper/plugin_module.h"
#include "content/renderer/pepper/v8_var_converter.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/render_frame_impl.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/object_template_builder.h"
#include "gin/public/gin_embedders.h"
#include "v8/include/v8.h"

#define X_LOG_V 0

using ppapi::ScopedPPVar;
namespace content {

namespace {

const char kPropertyParentalRatingSchems[] = "parentalRatingSchemes";

CbipPluginDispatcher* dispatcher() {
  if (RenderThreadImpl::current())
    return RenderThreadImpl::current()->cbip_plugin_dispatcher();
  return nullptr;
}
}  // end namespace

CbipPluginObjectParentalControlManager::CbipPluginObjectParentalControlManager
    (PepperPluginInstanceImpl* instance)
    : gin::NamedPropertyInterceptor(instance->GetIsolate(), this),
      instance_(instance),
      weak_ptr_factory_(this) {
  if (dispatcher()) {
    ipc_ = dispatcher()->GetAppMgr(cbip_instance_id_);
    const int routing_id = instance_->render_frame()->GetRoutingID();
    ipc_->BindParentalControlManager(routing_id);
  } else {
    ipc_ = nullptr;
  }
  if (instance_) {
    instance_->AddCbipPluginObject(this);
    v8::Isolate* isolate = instance_->GetIsolate();
    CbipObjectOipfParentalRatingSchemeCollectionClass* scheme_collection =
        new CbipObjectOipfParentalRatingSchemeCollectionClass(this);
    parental_rating_schemes_.Reset(
        isolate,
        scheme_collection->GetWrapper(isolate));
  }
}

CbipPluginObjectParentalControlManager::
    ~CbipPluginObjectParentalControlManager() {
  VLOG(X_LOG_V) << "ParentalControlManager::~ParentalControlManager";
  if (instance_) {
    instance_->RemoveCbipPluginObject(this);

    v8::Isolate* isolate = instance_->GetIsolate();
    v8::HandleScope handle_scope(isolate);
    CbipObjectOipfParentalRatingSchemeCollectionClass* p_parental_control_mgr =
        CbipObjectOipfParentalRatingSchemeCollectionClass::FromV8Object(isolate,
            v8::Local<v8::Object>::New(isolate, parental_rating_schemes_));
    DCHECK(p_parental_control_mgr);
    p_parental_control_mgr->OwnerDeleted();
  }
  if (dispatcher())
    dispatcher()->ReleaseAppMgr(cbip_instance_id_, ipc_.get());
}

// static
gin::WrapperInfo CbipPluginObjectParentalControlManager::kWrapperInfo =
    {gin::kEmbedderNativeGin};

// static
CbipPluginObjectParentalControlManager*
    CbipPluginObjectParentalControlManager::FromV8Object(
        v8::Isolate* isolate, v8::Handle<v8::Object> v8_object) {
  CbipPluginObjectParentalControlManager* configuration_plugin_object;
  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &configuration_plugin_object)) {
    return configuration_plugin_object;
  }
  return nullptr;
}

// static
PP_Var CbipPluginObjectParentalControlManager::Create(
    PepperPluginInstanceImpl* instance) {
  V8VarConverter var_converter(instance->pp_instance(),
  V8VarConverter::kAllowObjectVars);
  PepperTryCatchVar try_catch(instance, &var_converter, nullptr);
  CbipPluginObjectParentalControlManager* parental_control_manager_plugin =
      new CbipPluginObjectParentalControlManager(instance);
  gin::Handle<CbipPluginObjectParentalControlManager> object =
      gin::CreateHandle(instance->GetIsolate(),
                        parental_control_manager_plugin);
  ScopedPPVar result = try_catch.FromV8(object.ToV8());
  DCHECK(!try_catch.HasException());

  return result.Release();
}

v8::Local<v8::Value> CbipPluginObjectParentalControlManager::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
    VLOG(X_LOG_V) << "CbipPluginObjectParentalControlManager::GetNamedProperty:"
                  << identifier;

    if (!instance_)
      return v8::Local<v8::Value>();
    if (kPropertyParentalRatingSchems == identifier)
      return v8::Local<v8::Object>::New(GetIsolate(), parental_rating_schemes_);
    return v8::Local<v8::Value>();
}

bool CbipPluginObjectParentalControlManager::SetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  // ParentalControlManager has only read only properties.
  if (kPropertyParentalRatingSchems == identifier)
    return true;

  return false;
}

std::vector<std::string> CbipPluginObjectParentalControlManager::
    EnumerateNamedProperties(v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kPropertyParentalRatingSchems);

  return result;
}

void CbipPluginObjectParentalControlManager::InstanceDeleted() {
  const int routing_id = instance_->render_frame()->GetRoutingID();
  ipc_->UnbindParentalControlManager(routing_id);
  instance_ = nullptr;
}

gin::ObjectTemplateBuilder CbipPluginObjectParentalControlManager::
    GetObjectTemplateBuilder(v8::Isolate* isolate) {
  return Wrappable<CbipPluginObjectParentalControlManager>::
      GetObjectTemplateBuilder(isolate).AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor* CbipPluginObjectParentalControlManager::
    asGinNamedPropertyInterceptor() {
  return this;
}

PepperPluginInstanceImpl* CbipPluginObjectParentalControlManager::instance() {
  if (instance_)
    return instance_;
  return nullptr;
}

CbipPluginIPC* CbipPluginObjectParentalControlManager::ipc() {
  if (ipc_)
    return ipc_.get();
  return nullptr;
}

v8::Isolate* CbipPluginObjectParentalControlManager::GetIsolate() {
  if (instance_)
    return instance_->GetIsolate();
  return 0;
}

}  // namespace content
