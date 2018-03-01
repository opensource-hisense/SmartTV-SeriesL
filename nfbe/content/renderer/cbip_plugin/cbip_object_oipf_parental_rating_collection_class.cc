// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_object_oipf_parental_rating_collection_class.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_parental_rating_class.h"
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
const char kMethodAddParentalRating[] = "addParentalRating";

namespace content {

CbipObjectOipfParentalRatingCollectionClass::
    CbipObjectOipfParentalRatingCollectionClass(
    CbipObjectOwner* owner,
    const peer::oipf::ParentalRatingCollection &parental_ratings)
    : gin::NamedPropertyInterceptor(owner->GetIsolate(), this),
      owner_(owner),
      weak_ptr_factory_(this) {
  length_ = parental_ratings.size();
  for (auto item : parental_ratings) {
    CbipObjectOipfParentalRatingClass* parental_rating_obj =
        new CbipObjectOipfParentalRatingClass(this, item);
    v8::Persistent<v8::Object> js_parental_rating;
    js_parental_rating.Reset(
        owner->GetIsolate(),
        parental_rating_obj->GetWrapper(owner->GetIsolate()));
    parental_ratings_.push_back(js_parental_rating);
  }
}

CbipObjectOipfParentalRatingCollectionClass::
    ~CbipObjectOipfParentalRatingCollectionClass() {
  VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingCollectionClass::dtor:"
                << " this=" << static_cast<void*>(this);
}

// static
gin::WrapperInfo CbipObjectOipfParentalRatingCollectionClass::kWrapperInfo =
    {gin::kEmbedderNativeGin};

// static
CbipObjectOipfParentalRatingCollectionClass*
    CbipObjectOipfParentalRatingCollectionClass::FromV8Object(
        v8::Isolate* isolate,
        v8::Handle<v8::Object> v8_object) {
  CbipObjectOipfParentalRatingCollectionClass* self;
  if (!v8_object.IsEmpty() &&
    gin::ConvertFromV8(isolate, v8_object, &self)) {
    return self;
  }
  return NULL;
}

v8::Local<v8::Value>
    CbipObjectOipfParentalRatingCollectionClass::GetNamedProperty(
        v8::Isolate* isolate,
        const std::string& identifier) {
  VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingCollectionClass::"
                << "GetNamedProperty:" << identifier;
  if (identifier == kPropertyLength) {
    return v8::Integer::New(isolate, length_);
  }

  // methods of CbipObjectOipfParentalRatingCollectionClass
  if (identifier == kMethodItem) {
    return gin::CreateFunctionTemplate(isolate,
    base::Bind(&CbipObjectOipfParentalRatingCollectionClass::Item,
    weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kMethodToString) {
    return gin::CreateFunctionTemplate(isolate,
      base::Bind(&CbipObjectOipfParentalRatingCollectionClass::ToString,
      weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kMethodAddParentalRating) {
    return gin::CreateFunctionTemplate(isolate,
      base::Bind(
          &CbipObjectOipfParentalRatingCollectionClass::AddParentalRating,
          weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  return v8::Local<v8::Value>();
}

bool CbipObjectOipfParentalRatingCollectionClass::SetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (!owner_)
    return false;

    if (identifier == kPropertyLength) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name length")));
    return true;
  }

  DCHECK(isolate == owner_->GetIsolate());

  return false;
}

std::vector<std::string>
    CbipObjectOipfParentalRatingCollectionClass::EnumerateNamedProperties(
      v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kPropertyLength);
  result.push_back(kMethodItem);
  result.push_back(kMethodAddParentalRating);
  // kMethodToString isn't enumerable
  return result;
}

void CbipObjectOipfParentalRatingCollectionClass::OwnerDeleted() {
  owner_ = 0;
}

void CbipObjectOipfParentalRatingCollectionClass::ToString(
    gin::Arguments* args) {
  std::string str_class_name("[object ParentalRatingCollection]");
  v8::Handle<v8::Value> result =
      v8::String::NewFromUtf8(
          args->isolate(),
          str_class_name.c_str(),
          v8::String::kNormalString,
          str_class_name.length());
  args->Return(result);
}

void CbipObjectOipfParentalRatingCollectionClass::AddParentalRating(
    gin::Arguments* args) {
  if (owner_) {
    v8::Handle<v8::Value> v_scheme;
    v8::Handle<v8::Value> v_name;
    v8::Handle<v8::Value> v_value;
    v8::Handle<v8::Value> v_labels;
    v8::Handle<v8::Value> v_region;

    if (args->Length() != 5) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingCollectionClass::"
                    << "AddParentalRating invalid arguments passed";
      args->ThrowError();
      return;
    }
    if ((!args->GetNext(&v_scheme)) &&
        (!v_scheme->IsString())) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingCollectionClass::"
                    << "AddParentalRating: error at " << __LINE__;
      args->ThrowError();
      return;
    }
    v8::String::Utf8Value utf8_scheme(v_scheme);
    const std::string scheme(*utf8_scheme);

    if ((!args->GetNext(&v_name)) &&
        (!v_name->IsString())) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingCollectionClass::"
                    << "AddParentalRating: error at " << __LINE__;
      args->ThrowError();
      return;
    }
    v8::String::Utf8Value utf8_name(v_name);
    const std::string name(*utf8_name);

    if ((!args->GetNext(&v_value)) &&
        (!v_value->IsInt32())) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingCollectionClass::"
                    << "AddParentalRating: error at " << __LINE__;
      args->ThrowError();
      return;
    }
    v8::Handle<v8::Integer> js_integer;
    int value = -1;
    if (!v_value->IsUndefined()) {
      js_integer = v8::Handle<v8::Integer>::Cast(v_value->ToInteger());
      value = js_integer->Value();
    }

    if ((!args->GetNext(&v_labels)) &&
        (!v_labels->IsUint32())) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingCollectionClass::"
                    << "AddParentalRating: error at " << __LINE__;
      args->ThrowError();
      return;
     }
    js_integer = v8::Handle<v8::Integer>::Cast(v_labels->ToInteger());
    const unsigned int labels = js_integer->Value();

    if ((!args->GetNext(&v_region)) &&
        (!v_region->IsString())) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingCollectionClass::"
                    << "AddParentalRating: error at " << __LINE__;
      args->ThrowError();
      return;
    }
    std::string region;
    if (!v_region->IsNull() && !v_region->IsUndefined()) {
      v8::String::Utf8Value utf8_region(v_region);
      region = *utf8_region;  // const std::string region(*utf8_region);
    }
    peer::oipf::ParentalRatingItem item(name, scheme, value, labels, region);
    const int routing_id = owner_->instance()->render_frame()->GetRoutingID();

    // ipc to peer to add the ParentalRating into the collection
    if (ipc())
      ipc()->AddParentalRating(routing_id, &item);

    CbipObjectOipfParentalRatingClass* parental_rating_obj =
        new CbipObjectOipfParentalRatingClass(this, item);
    v8::Persistent<v8::Object> js_parental_rating;
    js_parental_rating.Reset(
        owner_->GetIsolate(),
        parental_rating_obj->GetWrapper(owner_->GetIsolate()));
    parental_ratings_.push_back(js_parental_rating);
    length_ = parental_ratings_.size();
  }  // end if (owner_)
}

void CbipObjectOipfParentalRatingCollectionClass::Item(gin::Arguments* args) {
  if (owner_) {
    v8::Handle<v8::Value> v_tmp;
    v8::Isolate* isolate = owner_->instance()->GetIsolate();

    if (args->Length() != 1) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingCollectionClass::Item "
                    << "invalid arguments passed";
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }

    if (!args->GetNext(&v_tmp)) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingCollectionClass::Item "
                    << "error at " << __LINE__;
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }
    if (!v_tmp->IsUint32()) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingCollectionClass::Item "
                    << "invalid argument";
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }
    v8::Handle<v8::Integer> js_index =
        v8::Handle<v8::Integer>::Cast(v_tmp->ToInteger());
    const unsigned int index = js_index->Value();
    if (length_ == 0  || index >= length_) {
      VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingCollectionClass::Item "
                    << "invalid argument";
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
      return;
    }

    v8::Local<v8::Object> object =
        v8::Local<v8::Object>::New(isolate, parental_ratings_[index]);
    CbipObjectOipfParentalRatingClass* parental_rating_obj =
        CbipObjectOipfParentalRatingClass::FromV8Object(isolate, object);
    if (parental_rating_obj) {
      v8::Persistent<v8::Object> js_parental_rating;
      js_parental_rating.Reset(
          isolate, parental_rating_obj->GetWrapper(isolate));
      args->Return(v8::Local<v8::Object>::New(isolate, js_parental_rating));
    } else {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
    }
  }
}

PepperPluginInstanceImpl*
    CbipObjectOipfParentalRatingCollectionClass::instance() {
  if (owner_)
    return owner_->instance();
  return nullptr;
}

CbipPluginIPC* CbipObjectOipfParentalRatingCollectionClass::ipc() {
  if (owner_)
    return owner_->ipc();
  return nullptr;
}

v8::Isolate* CbipObjectOipfParentalRatingCollectionClass::GetIsolate() {
  if (owner_)
    return owner_->GetIsolate();
  return 0;
}

gin::ObjectTemplateBuilder
    CbipObjectOipfParentalRatingCollectionClass::GetObjectTemplateBuilder(
        v8::Isolate* isolate) {
  return Wrappable<CbipObjectOipfParentalRatingCollectionClass>::
      GetObjectTemplateBuilder(isolate).AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor*
CbipObjectOipfParentalRatingCollectionClass::asGinNamedPropertyInterceptor() {
  return this;
}

}  // namespace content
