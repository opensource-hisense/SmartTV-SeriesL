// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_object_oipf_string_collection_class.h"

#include "access/peer/include/plugin/dae/video_broadcast_plugin.h"
#include "base/bind.h"
#include "base/logging.h"
#include "content/child/v8_value_converter_impl.h"
#include "content/renderer/cbip_plugin/cbip_object_owner.h"
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

CbipObjectOipfStringCollectionClass::CbipObjectOipfStringCollectionClass(
    CbipObjectOwner* owner, peer::oipf::StringCollectionData string_collection)
    : gin::NamedPropertyInterceptor(owner->GetIsolate(), this),
      owner_(owner),
      weak_ptr_factory_(this) {
  length_ = string_collection.size();
  string_collection_ = string_collection;
}

CbipObjectOipfStringCollectionClass::~CbipObjectOipfStringCollectionClass() {
    VLOG(X_LOG_V) << "CbipObjectOipfStringCollectionClass::dtor.";
}

// static
gin::WrapperInfo CbipObjectOipfStringCollectionClass::kWrapperInfo =
    {gin::kEmbedderNativeGin};

// static
CbipObjectOipfStringCollectionClass*
    CbipObjectOipfStringCollectionClass::FromV8Object(
        v8::Isolate* isolate,
        v8::Handle<v8::Object> v8_object) {
  CbipObjectOipfStringCollectionClass* self;
  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &self)) {
    return self;
  }
  return NULL;
}

v8::Local<v8::Value> CbipObjectOipfStringCollectionClass::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  VLOG(X_LOG_V) << "CbipObjectOipfStringCollectionClass::GetNamedProperty: "
                << identifier;

  if (identifier == kPropertyLength) {
    return v8::Integer::New(isolate, length_);
  }

  // methods of CbipObjectOipfStringCollectionClass
  if (identifier == kMethodItem) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipObjectOipfStringCollectionClass::Item,
        weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kMethodToString) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipObjectOipfStringCollectionClass::ToString,
        weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  return v8::Local<v8::Value>();
}

bool CbipObjectOipfStringCollectionClass::SetNamedProperty(v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (!owner_)
    return false;

  // The CbipObjectOipfStringCollectionClass has only readonly properties
  // as membders
  DCHECK(isolate == owner_->GetIsolate());

  return false;
}

std::vector<std::string>
    CbipObjectOipfStringCollectionClass::EnumerateNamedProperties(
        v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kPropertyLength);
  result.push_back(kMethodItem);
  // kMethodToString isn't enumerable
  return result;
}

void CbipObjectOipfStringCollectionClass::OwnerDeleted() {
  owner_ = 0;
}

void CbipObjectOipfStringCollectionClass::ToString(gin::Arguments* args) {
  std::string str_class_name("[object StringCollection]");
  v8::Handle<v8::Value> result =
      v8::String::NewFromUtf8(args->isolate(),
                              str_class_name.c_str(),
                              v8::String::kNormalString,
                              str_class_name.length());
  args->Return(result);
}

void CbipObjectOipfStringCollectionClass::Item(gin::Arguments* args) {
  v8::Handle<v8::Value> v_tmp;
  v8::Isolate* isolate = owner_->instance()->GetIsolate();

  if (!args->GetNext(&v_tmp)) {
    VLOG(X_LOG_V) << "CbipObjectOipfStringCollectionClass::Item:"
        << " error at " << __LINE__;
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }
  if (!v_tmp->IsUint32()) {
    VLOG(X_LOG_V) << "CbipObjectOipfStringCollectionClass::Item:"
        << " error at " << __LINE__;
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }

  v8::Handle<v8::Integer> js_index =
      v8::Handle<v8::Integer>::Cast(v_tmp->ToInteger());
  const size_t index = js_index->Uint32Value();

  if (index >= length_) {
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }
  v8::Local<v8::Value> result =
      v8::Local<v8::String>::New(isolate,
          gin::StringToV8(isolate, string_collection_.at(index)));
  args->Return(result);
}

gin::ObjectTemplateBuilder
    CbipObjectOipfStringCollectionClass::GetObjectTemplateBuilder(
        v8::Isolate* isolate) {
  return Wrappable<CbipObjectOipfStringCollectionClass>::
      GetObjectTemplateBuilder(isolate).AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor*
    CbipObjectOipfStringCollectionClass::asGinNamedPropertyInterceptor() {
  return this;
}

}  // namespace content
