// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_object_oipf_programme_collection_class.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_programme_class.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_video_broadcast.h"
#include "access/peer/include/plugin/dae/video_broadcast_plugin.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "content/child/v8_value_converter_impl.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "gin/public/gin_embedders.h"
#include "v8/include/v8.h"


#define X_LOG_V 0

const char kPropertyLength[] = "length";

const char kMethodItem[] = "item";
const char kMethodToString[] = "toString";

namespace content {

CbipObjectOipfProgrammeCollectionClass::CbipObjectOipfProgrammeCollectionClass(
    CbipObjectOwner* owner)
    : gin::NamedPropertyInterceptor(owner->GetIsolate(), this),
      owner_(owner),
      weak_ptr_factory_(this) {
  if (owner_ && owner_->instance() && owner_->ipc()) {
    int routing_id = owner_->instance()->render_frame()->GetRoutingID();
    length_ = owner_->ipc()->GetNumProgrammes(routing_id);
  } else {
    length_ = 0;  // If the owner is null length should be 0
  }
}

CbipObjectOipfProgrammeCollectionClass::
    ~CbipObjectOipfProgrammeCollectionClass() {
  VLOG(X_LOG_V) << "CbipObjectOipfProgrammeCollectionClass::dtor:"
                << " this=" << static_cast<void*>(this);
}

// static
gin::WrapperInfo CbipObjectOipfProgrammeCollectionClass::kWrapperInfo =
    {gin::kEmbedderNativeGin};

// static
CbipObjectOipfProgrammeCollectionClass*
    CbipObjectOipfProgrammeCollectionClass::FromV8Object(
        v8::Isolate* isolate,
        v8::Handle<v8::Object> v8_object) {
  CbipObjectOipfProgrammeCollectionClass* self;
  if (!v8_object.IsEmpty() &&
    gin::ConvertFromV8(isolate, v8_object, &self)) {
    return self;
  }
  return NULL;
}

v8::Local<v8::Value> CbipObjectOipfProgrammeCollectionClass::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  VLOG(X_LOG_V) << "CbipObjectOipfProgrammeCollectionClass::GetNamedProperty:"
                << identifier;

  if (identifier == kPropertyLength) {
    return v8::Integer::New(isolate, length_);
  }

  // Methods of CbipObjectOipfProgrammeCollectionClass
  if (identifier == kMethodItem) {
    return gin::CreateFunctionTemplate(isolate,
    base::Bind(&CbipObjectOipfProgrammeCollectionClass::Item,
    weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kMethodToString) {
    return gin::CreateFunctionTemplate(isolate,
      base::Bind(&CbipObjectOipfProgrammeCollectionClass::ToString,
      weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  return v8::Local<v8::Value>();
}

bool CbipObjectOipfProgrammeCollectionClass::SetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (!owner_)
    return false;

  // The CbipObjectOipfProgrammeCollectionClass has only constants
  // and readonly properties as members

  DCHECK(isolate == owner_->GetIsolate());

  return false;
}

std::vector<std::string>
    CbipObjectOipfProgrammeCollectionClass::EnumerateNamedProperties(
        v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kPropertyLength);
  result.push_back(kMethodItem);
  // kMethodToString isn't enumerable
  return result;
}

void CbipObjectOipfProgrammeCollectionClass::OwnerDeleted() {
  owner_ = 0;
}

void CbipObjectOipfProgrammeCollectionClass::ToString(gin::Arguments* args) {
  std::string str_class_name("[object ProgrammeCollection]");
  v8::Handle<v8::Value> result =
      v8::String::NewFromUtf8(args->isolate(), str_class_name.c_str(),
      v8::String::kNormalString, str_class_name.length());
  args->Return(result);
}

void CbipObjectOipfProgrammeCollectionClass::Item(gin::Arguments* args) {
  if (owner_ && owner_->instance()) {
    v8::Handle<v8::Value> v_tmp;
    if (!args->GetNext(&v_tmp)) {
      VLOG(X_LOG_V) << "CbipObjectOipfProgrammeCollectionClass::Item error at "
                    << __LINE__;
      args->ThrowError();
      return;
    }
    if (!v_tmp->IsInt32()) {
      VLOG(X_LOG_V) << "CbipObjectOipfProgrammeCollectionClass::Item error at "
                    << __LINE__;
      args->ThrowError();
      return;
    }
    v8::Handle<v8::Integer> js_index =
        v8::Handle<v8::Integer>::Cast(v_tmp->ToInteger());
    const int index = js_index->Value();

    v8::Isolate* isolate = owner_->instance()->GetIsolate();
    int routing_id = owner_->instance()->render_frame()->GetRoutingID();

    if (0 > index || index >= length_) {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
      return;
    }

    bool is_success;
    peer::oipf::VideoBroadcastProgramme programme;
    owner_->ipc()->GetProgrammeItemByIndex(routing_id, index,
                                           &is_success, &programme);
    if (!is_success) {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
      return;
    }
    CbipObjectOipfProgrammeClass* programme_obj =
        new CbipObjectOipfProgrammeClass(this, programme);
    v8::Persistent<v8::Object> js_programme;
    js_programme.Reset(isolate, programme_obj->GetWrapper(isolate));

    args->Return(v8::Local<v8::Object>::New(isolate, js_programme));
  }
}

PepperPluginInstanceImpl* CbipObjectOipfProgrammeCollectionClass::instance() {
  if (owner_)
    return owner_->instance();
  return nullptr;
}

CbipPluginIPC* CbipObjectOipfProgrammeCollectionClass::ipc() {
  if (owner_)
    return owner_->ipc();
  return nullptr;
}


v8::Isolate* CbipObjectOipfProgrammeCollectionClass::GetIsolate() {
  if (owner_)
    return owner_->GetIsolate();
  return nullptr;
}

gin::ObjectTemplateBuilder
    CbipObjectOipfProgrammeCollectionClass::GetObjectTemplateBuilder(
        v8::Isolate* isolate) {
  return Wrappable<CbipObjectOipfProgrammeCollectionClass>::
      GetObjectTemplateBuilder(isolate).AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor*
    CbipObjectOipfProgrammeCollectionClass::asGinNamedPropertyInterceptor() {
  return this;
}

}  // namespace content

