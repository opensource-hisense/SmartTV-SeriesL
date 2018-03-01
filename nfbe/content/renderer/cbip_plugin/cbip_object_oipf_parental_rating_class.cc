// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_object_oipf_parental_rating_class.h"
#include <vector>

#include "access/peer/include/plugin/dae/video_broadcast_plugin.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "gin/public/gin_embedders.h"
#include "v8/include/v8.h"

#define X_LOG_V 0

// ParentalRating Class properties.
const char kName[] = "name";
const char kScheme[] = "scheme";
const char kValue[] = "value";
const char kLabels[] = "labels";
const char kRegion[] = "region";
// ParentalRating Class Method.
const char kMethodToString[] = "toString";

namespace content {

CbipObjectOipfParentalRatingClass::CbipObjectOipfParentalRatingClass(
    CbipObjectOwner* owner,
    const peer::oipf::ParentalRatingItem& parental_rating)
    : gin::NamedPropertyInterceptor(owner->GetIsolate(), this),
      owner_(owner),
      weak_ptr_factory_(this) {
  name_ = parental_rating.name;
  scheme_ = parental_rating.scheme;
  value_ = parental_rating.value;
  labels_ = parental_rating.labels;
  region_ = parental_rating.region;
}

CbipObjectOipfParentalRatingClass::~CbipObjectOipfParentalRatingClass() {
    VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingClass::dtor:"
                  << " this=" << static_cast<void*>(this);
}

// static
gin::WrapperInfo CbipObjectOipfParentalRatingClass::kWrapperInfo =
    {gin::kEmbedderNativeGin};

// static
CbipObjectOipfParentalRatingClass*
    CbipObjectOipfParentalRatingClass::FromV8Object(
        v8::Isolate* isolate,
        v8::Handle<v8::Object> v8_object) {
  CbipObjectOipfParentalRatingClass* self;
  if (!v8_object.IsEmpty() &&
    gin::ConvertFromV8(isolate, v8_object, &self)) {
    return self;
  }
  return NULL;
}

v8::Local<v8::Value> CbipObjectOipfParentalRatingClass::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  VLOG(X_LOG_V) << "CbipObjectOipfParentalRatingClass::GetNamedProperty:"
                << identifier;

  // properties of CbipObjectOipfParentalRatingClass class
  if (identifier == kName) {
    return v8::Local<v8::String>::New(isolate,
                                      gin::StringToV8(isolate, name_));
  }
  if (identifier == kScheme) {
    return v8::Local<v8::String>::New(isolate,
                                      gin::StringToV8(isolate, scheme_));
  }
  if (identifier == kValue) {
       return v8::Integer::New(isolate, value_);;
  }
  if (identifier == kRegion) {
    return v8::Local<v8::String>::New(isolate,
                                      gin::StringToV8(isolate, region_));
  }
  if (identifier == kLabels) {
    return v8::Integer::New(isolate, labels_);
  }

  // methods of CbipObjectOipfParentalRatingClass
  if (identifier == kMethodToString) {
    return gin::CreateFunctionTemplate(isolate,
      base::Bind(&CbipObjectOipfParentalRatingClass::ToString,
      weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  return v8::Local<v8::Value>();
}

bool CbipObjectOipfParentalRatingClass::SetNamedProperty(v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (!owner_)
    return false;
  DCHECK(isolate == owner_->GetIsolate());

  if (identifier == kName ||
      identifier == kScheme ||
      identifier == kValue ||
      identifier == kLabels ||
      identifier == kRegion) {
    return true;
  }

  // The CbipObjectOipfParentalRatingClass has only readonly properties

  return false;
}

std::vector<std::string>
    CbipObjectOipfParentalRatingClass::EnumerateNamedProperties(
        v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kName);
  result.push_back(kScheme);
  result.push_back(kValue);
  result.push_back(kLabels);
  result.push_back(kRegion);
  // kMethodToString isn't enumerable
  return result;
}

void CbipObjectOipfParentalRatingClass::OwnerDeleted() {
  owner_ = 0;
}

void CbipObjectOipfParentalRatingClass::ToString(gin::Arguments* args) {
  std::string str_class_name("[object ParentalRating]");
  v8::Handle<v8::Value> result =
      v8::String::NewFromUtf8(
          args->isolate(),
          str_class_name.c_str(),
          v8::String::kNormalString,
          str_class_name.length());
  args->Return(result);
}

gin::ObjectTemplateBuilder
    CbipObjectOipfParentalRatingClass::GetObjectTemplateBuilder(
        v8::Isolate* isolate) {
  return Wrappable<CbipObjectOipfParentalRatingClass>::
             GetObjectTemplateBuilder(isolate).AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor*
    CbipObjectOipfParentalRatingClass::asGinNamedPropertyInterceptor() {
  return this;
}

void CbipObjectOipfParentalRatingClass::TestConvertToStruct(
    CbipObjectOipfParentalRatingClass* p_value,
	peer::oipf::ParentalRatingItem *out) {
  if(p_value && out) {
    out->name = p_value->name_;
    out->scheme = p_value->scheme_;
    out->value = p_value->value_;
    out->labels = p_value->labels_;
    out->region = p_value->region_;
  }
}

}  // namespace content

