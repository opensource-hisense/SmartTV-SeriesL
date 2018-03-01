// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_object_oipf_parental_rating_scheme_collection_class.h"
#include "access/peer/include/plugin/dae/parental_control_manager_plugin.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_parental_rating_scheme_class.h"
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
const char kMethodGetParentalRating[] = "getParentalRatingScheme";

namespace content {

CbipObjectOipfParentalRatingSchemeCollectionClass::
    CbipObjectOipfParentalRatingSchemeCollectionClass(
    CbipObjectOwner* owner)
    : gin::NamedPropertyInterceptor(owner->GetIsolate(), this),
      owner_(owner),
      weak_ptr_factory_(this) {
  if (owner_ && owner_->instance() && owner_->ipc()) {
    int routing_id = owner_->instance()->render_frame()->GetRoutingID();
    length_ = owner_->ipc()->GetNumOfParentralRatingSchemes(routing_id);
  } else {
    length_ = 0;
  }
}

CbipObjectOipfParentalRatingSchemeCollectionClass::
    ~CbipObjectOipfParentalRatingSchemeCollectionClass() {
  VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingSchemeCollectionClass::dtor:";
}

// static
gin::WrapperInfo
    CbipObjectOipfParentalRatingSchemeCollectionClass::kWrapperInfo =
        {gin::kEmbedderNativeGin};

// static
CbipObjectOipfParentalRatingSchemeCollectionClass*
    CbipObjectOipfParentalRatingSchemeCollectionClass::FromV8Object(
        v8::Isolate* isolate,
        v8::Handle<v8::Object> v8_object) {
  CbipObjectOipfParentalRatingSchemeCollectionClass* self;
  if (!v8_object.IsEmpty() &&
    gin::ConvertFromV8(isolate, v8_object, &self)) {
    return self;
  }
  return nullptr;
}

v8::Local<v8::Value>
    CbipObjectOipfParentalRatingSchemeCollectionClass::GetNamedProperty(
        v8::Isolate* isolate,
        const std::string& identifier) {
  VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingSchemeCollectionClass::"
                << "GetNamedProperty:" << identifier;
  if (identifier == kPropertyLength) {
    return v8::Integer::New(isolate, length_);
  }

  // methods of CbipObjectOipfParentalRatingSchemeCollectionClass
  if (identifier == kMethodItem) {
    return gin::CreateFunctionTemplate(isolate,
    base::Bind(&CbipObjectOipfParentalRatingSchemeCollectionClass::item,
    weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kMethodToString) {
    return gin::CreateFunctionTemplate(isolate,
      base::Bind(&CbipObjectOipfParentalRatingSchemeCollectionClass::toString,
      weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kMethodGetParentalRating) {
    return gin::CreateFunctionTemplate(isolate,
      base::Bind(&CbipObjectOipfParentalRatingSchemeCollectionClass::
                 getParentalRatingScheme,
                 weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  return v8::Local<v8::Value>();
}

bool CbipObjectOipfParentalRatingSchemeCollectionClass::SetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (!owner_)
    return false;

  DCHECK(isolate == owner_->GetIsolate());

  return false;
}

std::vector<std::string>
    CbipObjectOipfParentalRatingSchemeCollectionClass::EnumerateNamedProperties(
      v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kPropertyLength);
  result.push_back(kMethodItem);
  result.push_back(kMethodGetParentalRating);
  // kMethodToString isn't enumerable
  return result;
}

void CbipObjectOipfParentalRatingSchemeCollectionClass::OwnerDeleted() {
  owner_ = 0;
}

void CbipObjectOipfParentalRatingSchemeCollectionClass::toString(
    gin::Arguments* args) {
  std::string str_class_name("[object ParentalRatingSchemeCollection]");
  v8::Handle<v8::Value> result =
      v8::String::NewFromUtf8(
          args->isolate(),
          str_class_name.c_str(),
          v8::String::kNormalString,
          str_class_name.length());
  args->Return(result);
}

void CbipObjectOipfParentalRatingSchemeCollectionClass::getParentalRatingScheme(
    gin::Arguments* args) {
  if (owner_) {
    v8::Handle<v8::Value> v_tmp;
    v8::Isolate* isolate = owner_->instance()->GetIsolate();
    int routing_id = owner_->instance()->render_frame()->GetRoutingID();
    if (!args->GetNext(&v_tmp)) {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }
    if (v_tmp->IsString()) {
      v8::String::Utf8Value js_name(v_tmp->ToString());
      const std::string scheme_name = std::string(*js_name);
      if (scheme_name.empty()) {
        args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
        return;
      }
      bool is_success = false;
      peer::oipf::ParentalRatingScheme parental_rating_scheme;
      owner_->ipc()->GetParentalRatingSchemeByName(routing_id, scheme_name,
        &is_success, &parental_rating_scheme);
      if (!is_success) {
        args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
        return;
      }
      CbipObjectOipfParentalRatingSchemeClass* scheme_obj =
          new CbipObjectOipfParentalRatingSchemeClass(this,
              parental_rating_scheme);
      v8::Persistent<v8::Object> js_scheme;
      js_scheme.Reset(isolate, scheme_obj->GetWrapper(isolate));
      args->Return(v8::Local<v8::Object>::New(isolate, js_scheme));
    } else {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
    }
  }
}

void CbipObjectOipfParentalRatingSchemeCollectionClass::item(
    gin::Arguments* args) {
  if (owner_) {
    v8::Handle<v8::Value> v_tmp;
    v8::Isolate* isolate = owner_->instance()->GetIsolate();
    int routing_id = owner_->instance()->render_frame()->GetRoutingID();

    if (args->Length() != 1) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingSchemeCollectionClass"
                    << "::Item invalid arguments passed";
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }

    if (!args->GetNext(&v_tmp)) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingSchemeCollectionClass"
                    << "::Item error at " << __LINE__;
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }
    if (!v_tmp->IsUint32()) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingSchemeCollectionClass"
                    << "::Item invalid argument";
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }
    v8::Handle<v8::Integer> js_index =
        v8::Handle<v8::Integer>::Cast(v_tmp->ToInteger());
    const uint32_t index = js_index->Uint32Value();
    if (length_ == 0  || index >= length_) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingSchemeCollectionClass"
                    << "::Item invalid argument";
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }

    bool is_success = false;
    peer::oipf::ParentalRatingScheme parental_rating_scheme;
    owner_->ipc()->GetParentalRatingSchemeByIndex(routing_id, index,
        &is_success, &parental_rating_scheme);
    if (!is_success) {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }
    CbipObjectOipfParentalRatingSchemeClass* scheme_obj =
        new CbipObjectOipfParentalRatingSchemeClass(this,
            parental_rating_scheme);
    v8::Persistent<v8::Object> js_scheme;
    js_scheme.Reset(isolate, scheme_obj->GetWrapper(isolate));
    args->Return(v8::Local<v8::Object>::New(isolate, js_scheme));
  }
}

PepperPluginInstanceImpl*
    CbipObjectOipfParentalRatingSchemeCollectionClass::instance() {
  if (owner_)
    return owner_->instance();
  return nullptr;
}

CbipPluginIPC* CbipObjectOipfParentalRatingSchemeCollectionClass::ipc() {
  if (owner_)
    return owner_->ipc();
  return nullptr;
}

v8::Isolate* CbipObjectOipfParentalRatingSchemeCollectionClass::GetIsolate() {
  if (owner_)
    return owner_->GetIsolate();
  return 0;
}

gin::ObjectTemplateBuilder
    CbipObjectOipfParentalRatingSchemeCollectionClass::GetObjectTemplateBuilder(
        v8::Isolate* isolate) {
  return Wrappable<CbipObjectOipfParentalRatingSchemeCollectionClass>::
      GetObjectTemplateBuilder(isolate).AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor*
    CbipObjectOipfParentalRatingSchemeCollectionClass::
        asGinNamedPropertyInterceptor() {
  return this;
}

}  // namespace content
