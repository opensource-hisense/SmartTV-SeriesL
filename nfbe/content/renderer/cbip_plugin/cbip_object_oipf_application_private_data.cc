// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.
#include "content/renderer/cbip_plugin/cbip_object_oipf_application_private_data.h"

#include "access/content/renderer/oipf/web_channel_impl.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_application.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_keyset.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/render_frame_impl.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "gin/public/gin_embedders.h"
#include "third_party/WebKit/public/web/modules/oipf/WebChannelUtil.h"
#include "v8/include/v8.h"

#define X_LOG_V 0

namespace content {

namespace {

const char kGetFreeMem[] = "getFreeMem";
const char kKeyset[] = "keyset";
const char kCurrentChannel[] = "currentChannel";

// ApplicationPrivateData class method.
const char kMethodToString[] = "toString";
}  // namespace

// only Application should call this function.
CbipObjectOipfApplicationPrivateData::CbipObjectOipfApplicationPrivateData(
    CbipObjectOipfApplication* owner)
    : gin::NamedPropertyInterceptor(owner->GetIsolate(), this),
      CbipObjectOwner(),
      owner_(owner),
      weak_ptr_factory_(this) {
  v8::Isolate* isolate = owner_->GetIsolate();
  CbipObjectOipfKeyset* kset =
      new CbipObjectOipfKeyset(this);
  keyset_.Reset(isolate, kset->GetWrapper(isolate));
}

CbipObjectOipfApplicationPrivateData::~CbipObjectOipfApplicationPrivateData() {
  VLOG(X_LOG_V) << "CbipObjectOipfApplicationPrivateData::dtor";
  keyset_.Reset();
}

// static
gin::WrapperInfo CbipObjectOipfApplicationPrivateData::kWrapperInfo =
    {gin::kEmbedderNativeGin};

// static
CbipObjectOipfApplicationPrivateData*
    CbipObjectOipfApplicationPrivateData::FromV8Object(
        v8::Isolate* isolate,
        v8::Handle<v8::Object> v8_object) {
  CbipObjectOipfApplicationPrivateData* self;
  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &self)) {
    return self;
  }
  return NULL;
}

v8::Local<v8::Value> CbipObjectOipfApplicationPrivateData::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  VLOG(X_LOG_V) << "CbipObjectOipfApplicationPrivateData::GetNamedProperty:"
                << " " << identifier;
  if (!owner_)
    return v8::Local<v8::Value>();
  if (!owner_->GetIsolate())
    return v8::Local<v8::Value>();
  DCHECK(isolate == owner_->GetIsolate());

  if (identifier == kGetFreeMem) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipObjectOipfApplicationPrivateData::getFreeMem,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kKeyset) {
    return v8::Local<v8::Object>::New(isolate, keyset_);
  }

  if (identifier == kCurrentChannel) {
    bool is_success;
    peer::oipf::VideoBroadcastChannel channel;
    const int routing_id = owner_->instance()->render_frame()->GetRoutingID();
    ipc()->GetCurrentChannel(routing_id, &is_success, &channel);
    if (is_success) {
      WebChannelImpl* web_channel_impl = new WebChannelImpl(channel);
      v8::Local<v8::Value> v8_channel =
          blink::WebChannelUtil::toV8Value(web_channel_impl, isolate);
      return v8_channel;
    }
    return v8::Null(isolate);
  }

  if (identifier == kMethodToString) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipObjectOipfApplicationPrivateData::ToString,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  return v8::Local<v8::Value>();
}

bool CbipObjectOipfApplicationPrivateData::SetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {

  if (identifier == kGetFreeMem) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name getFreeMem")));
    return true;
  }
  if (identifier == kKeyset) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name keyset")));
    return true;
  }
  if (identifier == kCurrentChannel) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name CurrentChannel")));
    return true;
  }

  return false;
}

std::vector<std::string>
    CbipObjectOipfApplicationPrivateData::EnumerateNamedProperties(
        v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kGetFreeMem);
  result.push_back(kKeyset);
  result.push_back(kCurrentChannel);
  // kMethodToString isn't enumerable.
  return result;
}

void CbipObjectOipfApplicationPrivateData::ToString(gin::Arguments* args) {
  std::string str_class_name("[object ApplicationPrivateData]");
  v8::Handle<v8::Value> result =
      v8::String::NewFromUtf8(
          args->isolate(),
          str_class_name.c_str(),
          v8::String::kNormalString,
          str_class_name.length());
  args->Return(result);
}

PepperPluginInstanceImpl* CbipObjectOipfApplicationPrivateData::instance() {
  if (owner_)
    return owner_->instance();
  return nullptr;
}

CbipPluginIPC* CbipObjectOipfApplicationPrivateData::ipc() {
  if (owner_)
    return owner_->ipc();
  return nullptr;
}

v8::Isolate* CbipObjectOipfApplicationPrivateData::GetIsolate() {
  if (owner_)
    return owner_->GetIsolate();
  return 0;
}

void CbipObjectOipfApplicationPrivateData::OwnerApplicationDeleted() {
  owner_ = 0;
}

gin::ObjectTemplateBuilder
    CbipObjectOipfApplicationPrivateData::GetObjectTemplateBuilder(
        v8::Isolate* isolate) {
  return Wrappable<CbipObjectOipfApplicationPrivateData>::
      GetObjectTemplateBuilder(isolate).AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor*
    CbipObjectOipfApplicationPrivateData::asGinNamedPropertyInterceptor() {
  return this;
}

void CbipObjectOipfApplicationPrivateData::getFreeMem(gin::Arguments* args) {
  // FIXME: one can obtain an ApplicationPrivate object and destroy the
  // Appliction object.  here we do not support that case.
  // after the Application object, from which this ApplicationPrivate object
  // is otained, is destroyed, this function returns an empty value.
  if (!owner_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  owner_->getFreeMem(args);
}

}  // namespace content
