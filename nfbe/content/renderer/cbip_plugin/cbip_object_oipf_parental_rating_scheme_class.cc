// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_object_oipf_parental_rating_scheme_class.h"
#include "access/peer/include/plugin/dae/video_broadcast_plugin.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_parental_rating_class.h"
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

// ParentalRatingScheme Class properties.
const char kPropertyName[] = "name";
const char kPropertyThreshold[] = "threshold";
const char kPropertyLength[] = "length";

// ParentalRatingScheme Class methods.
const char kMethodIndexOf[] = "indexOf";
const char kMethodIconUri[] = "iconUri";
const char kMethodItem[] = "item";
const char kMethodToString[] = "toString";

namespace content {

CbipObjectOipfParentalRatingSchemeClass::
    CbipObjectOipfParentalRatingSchemeClass(CbipObjectOwner* owner,
        const peer::oipf::ParentalRatingScheme& scheme)
    : gin::NamedPropertyInterceptor(owner->GetIsolate(), this),
      owner_(owner),
      weak_ptr_factory_(this) {
  name_  = scheme.name;
  CbipObjectOipfParentalRatingClass* parental_rating_obj =
      new CbipObjectOipfParentalRatingClass(this, scheme.threshold);
  threshold_.Reset(owner_->GetIsolate(),
      parental_rating_obj->GetWrapper(owner_->GetIsolate()));
  parental_rating_scheme_items = scheme.parental_rating_scheme_items;
  if (name_ == "dvb-si")
    length_ = 0;
  else
    length_ = parental_rating_scheme_items.size();
}

CbipObjectOipfParentalRatingSchemeClass::
    ~CbipObjectOipfParentalRatingSchemeClass() {
  VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingSchemeClass::dtor:";
}

// static
gin::WrapperInfo
    CbipObjectOipfParentalRatingSchemeClass::kWrapperInfo =
        {gin::kEmbedderNativeGin};

// static
CbipObjectOipfParentalRatingSchemeClass*
    CbipObjectOipfParentalRatingSchemeClass::FromV8Object(
        v8::Isolate* isolate,
        v8::Handle<v8::Object> v8_object) {
  CbipObjectOipfParentalRatingSchemeClass* self;
  if (!v8_object.IsEmpty() &&
    gin::ConvertFromV8(isolate, v8_object, &self)) {
    return self;
  }
  return nullptr;
}

v8::Local<v8::Value>
    CbipObjectOipfParentalRatingSchemeClass::GetNamedProperty(
        v8::Isolate* isolate,
        const std::string& identifier) {
  VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingSchemeClass::"
                << "GetNamedProperty:" << identifier;
  if (identifier == kPropertyName) {
    return v8::Local<v8::String>::New(isolate,
        gin::StringToV8(isolate, name_));
  }
  if (identifier == kPropertyThreshold) {
    return v8::Local<v8::Object>::New(isolate, threshold_);
  }
  if (identifier == kPropertyLength) {
    return v8::Integer::New(isolate, length_);
  }
  if (identifier == kMethodIndexOf) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipObjectOipfParentalRatingSchemeClass::indexOf,
        weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kMethodIconUri) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipObjectOipfParentalRatingSchemeClass::iconUri,
        weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kMethodItem) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipObjectOipfParentalRatingSchemeClass::item,
        weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kMethodToString) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipObjectOipfParentalRatingSchemeClass::toString,
        weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  return v8::Local<v8::Value>();
}

bool CbipObjectOipfParentalRatingSchemeClass::SetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (!owner_)
    return false;
  if (identifier == kPropertyName ||
      identifier == kPropertyThreshold ||
      identifier == kPropertyLength) {
    // The CbipObjectOipfParentalRatingSchemeClass has only readonly
    // properties as members
    return true;
  }
  DCHECK(isolate == owner_->GetIsolate());

  return false;
}

std::vector<std::string>
    CbipObjectOipfParentalRatingSchemeClass::EnumerateNamedProperties(
      v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kPropertyName);
  result.push_back(kPropertyThreshold);
  result.push_back(kPropertyLength);
  result.push_back(kMethodIndexOf);
  result.push_back(kMethodIconUri);
  result.push_back(kMethodItem);
  // kMethodToString isn't enumerable
  return result;
}

void CbipObjectOipfParentalRatingSchemeClass::OwnerDeleted() {
  owner_ = 0;
}

void CbipObjectOipfParentalRatingSchemeClass::indexOf(
    gin::Arguments* args) {
  v8::Isolate* isolate = owner_->instance()->GetIsolate();
  if (owner_) {
    v8::Handle<v8::Value> v_tmp;
    if (!args->GetNext(&v_tmp)) {
      args->Return(static_cast<v8::Local<v8::Value>>(
          v8::Integer::New(isolate, -1)));
      return;
    }
    if (v_tmp->IsString()) {
      v8::String::Utf8Value js_name(v_tmp->ToString());
      const std::string scheme_name = std::string(*js_name);
      if (scheme_name.empty()) {
        args->Return(static_cast<v8::Local<v8::Value>>(
            v8::Integer::New(isolate, -1)));
        return;
      }
      for (auto i = 0; i < parental_rating_scheme_items.size(); i++) {
        if (scheme_name == parental_rating_scheme_items[i].name) {
          args->Return(static_cast<v8::Local<v8::Value>>(
              v8::Integer::New(isolate, i)));
          return;
        }
      }
    }
  }
  args->Return(static_cast<v8::Local<v8::Value>>(
      v8::Integer::New(isolate, -1)));
}

void CbipObjectOipfParentalRatingSchemeClass::iconUri(
    gin::Arguments* args) {
  if (owner_) {
    v8::Handle<v8::Value> v_tmp;
    v8::Isolate* isolate = owner_->instance()->GetIsolate();

    if (args->Length() != 1) {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }

    if (!args->GetNext(&v_tmp)) {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }
    if (!v_tmp->IsUint32()) {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }
    v8::Handle<v8::Integer> js_index =
        v8::Handle<v8::Integer>::Cast(v_tmp->ToInteger());
    const unsigned int index = js_index->Value();
    if (parental_rating_scheme_items.size() == 0  ||
        index >= parental_rating_scheme_items.size()) {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }
    if ((parental_rating_scheme_items[index].image_uri).empty())
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
    else
      args->Return(static_cast<v8::Local<v8::Value>>(
          v8::Local<v8::String>::New(isolate,
              gin::StringToV8(isolate,
                  parental_rating_scheme_items[index].image_uri))));
  }
}

void CbipObjectOipfParentalRatingSchemeClass::item(
    gin::Arguments* args) {
  if (instance()) {
    v8::Handle<v8::Value> v_tmp;
    v8::Isolate* isolate = GetIsolate();

    if (!args->GetNext(&v_tmp)) {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }
    if (!v_tmp->IsUint32()) {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }
    v8::Handle<v8::Uint32> js_index =
        v8::Handle<v8::Uint32>::Cast(v_tmp->ToUint32());
    const unsigned int index = js_index->Value();
    if (parental_rating_scheme_items.size() == 0  ||
        index >= parental_rating_scheme_items.size()) {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }

    if ((parental_rating_scheme_items[index].name).empty())
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
    else
      args->Return(static_cast<v8::Local<v8::Value>>(
          v8::Local<v8::String>::New(isolate,
              gin::StringToV8(isolate,
                  parental_rating_scheme_items[index].name))));
  }
}

void CbipObjectOipfParentalRatingSchemeClass::toString(
    gin::Arguments* args) {
  std::string str_class_name("[object ParentalRatingScheme]");
  v8::Handle<v8::Value> result =
      v8::String::NewFromUtf8(
          args->isolate(),
          str_class_name.c_str(),
          v8::String::kNormalString,
          str_class_name.length());
  args->Return(result);
}

PepperPluginInstanceImpl*
    CbipObjectOipfParentalRatingSchemeClass::instance() {
  if (owner_)
    return owner_->instance();
  return nullptr;
}

CbipPluginIPC* CbipObjectOipfParentalRatingSchemeClass::ipc() {
  if (owner_)
    return owner_->ipc();
  return nullptr;
}

v8::Isolate* CbipObjectOipfParentalRatingSchemeClass::GetIsolate() {
  if (owner_)
    return owner_->GetIsolate();
  return 0;
}

gin::ObjectTemplateBuilder
    CbipObjectOipfParentalRatingSchemeClass::GetObjectTemplateBuilder(
        v8::Isolate* isolate) {
  return Wrappable<CbipObjectOipfParentalRatingSchemeClass>::
      GetObjectTemplateBuilder(isolate).AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor*
    CbipObjectOipfParentalRatingSchemeClass::
        asGinNamedPropertyInterceptor() {
  return this;
}

}  // namespace content
