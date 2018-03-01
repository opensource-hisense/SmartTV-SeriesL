// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_object_oipf_programme_class.h"

#include "access/peer/include/plugin/dae/video_broadcast_plugin.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_parental_rating_collection_class.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_string_collection_class.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "gin/public/gin_embedders.h"
#include "v8/include/v8.h"

#define X_LOG_V 0

// programme class constants.
const char kID_TVA_CRID[] = "ID_TVA_CRID";
const char kID_DVB_EVENT[] = "ID_DVB_EVENT";
const char kID_TVA_GROUP_CRID[] = "ID_TVA_GROUP_CRID";

// programme class properties.
const char kName[] = "name";
const char kProgrammeID[] = "programmeID";
const char kProgrammeIDType[] = "programmeIDType";
const char kDescription[] = "description";
const char kLongDescription[] = "longDescription";
const char kStartTime[] = "startTime";
const char kDuration[] = "duration";
const char kChannelID[] = "channelID";
const char kParentalRatings[] = "parentalRatings";

const char kMethodGetSIDescriptors[] = "getSIDescriptors";
const char kMethodToString[] = "toString";

namespace content {


CbipObjectOipfProgrammeClass::CbipObjectOipfProgrammeClass(
    CbipObjectOwner* owner,
    const peer::oipf::VideoBroadcastProgramme& programme)
    : gin::NamedPropertyInterceptor(owner->GetIsolate(), this),
      owner_(owner),
      weak_ptr_factory_(this) {
  name_ = programme.name;
  programme_id_ = programme.id;
  programme_id_type_ = programme.programme_id_type;
  description_ = programme.long_description;
  long_description_ = programme.description;
  start_time_ = programme.start_time;
  duration_ = programme.duration;
  channel_id_ = programme.channel_id;
  CbipObjectOipfParentalRatingCollectionClass* parental_rating_collection =
      new CbipObjectOipfParentalRatingCollectionClass(
          this, programme.parental_rating_collection);
  parental_rating_collection_.Reset(
      owner->GetIsolate(),
      parental_rating_collection->GetWrapper(owner->GetIsolate()));
}

CbipObjectOipfProgrammeClass::~CbipObjectOipfProgrammeClass() {
    VLOG(X_LOG_V) << "CbipObjectOipfProgrammeClass::dtor: this="
                  << static_cast<void*>(this);
}

// static
gin::WrapperInfo CbipObjectOipfProgrammeClass::kWrapperInfo =
    {gin::kEmbedderNativeGin};

// static
CbipObjectOipfProgrammeClass* CbipObjectOipfProgrammeClass::FromV8Object(
    v8::Isolate* isolate,
    v8::Handle<v8::Object> v8_object) {
  CbipObjectOipfProgrammeClass* self;
  if (!v8_object.IsEmpty() &&
    gin::ConvertFromV8(isolate, v8_object, &self)) {
    return self;
  }
  return NULL;
}

v8::Local<v8::Value> CbipObjectOipfProgrammeClass::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  VLOG(X_LOG_V) << "CbipObjectOipfProgrammeClass::GetNamedProperty:"
                << identifier;

  if (identifier == kID_TVA_CRID) {
    return v8::Integer::New(isolate, 0x0);
  }
  if (identifier == kID_DVB_EVENT) {
    return v8::Integer::New(isolate, 0x1);
  }
  if (identifier == kID_TVA_GROUP_CRID) {
    return v8::Integer::New(isolate, 0x2);
  }

  // properties of CbipObjectOipfProgrammeClass class
  if (identifier == kName) {
    return v8::Local<v8::String>::New(isolate, gin::StringToV8(isolate, name_));
  }
  if (identifier == kProgrammeID) {
    return v8::Local<v8::String>::New(isolate,
        gin::StringToV8(isolate, programme_id_));
  }
  if (identifier == kProgrammeIDType) {
       return v8::Integer::New(isolate, programme_id_type_);;
  }
  if (identifier == kDescription) {
    return v8::Local<v8::String>::New(isolate,
        gin::StringToV8(isolate, description_));
  }
  if (identifier == kLongDescription) {
    return v8::Local<v8::String>::New(isolate,
        gin::StringToV8(isolate, long_description_));
  }
  if (identifier == kStartTime) {
    return v8::Number::New(isolate, start_time_);
  }
  if (identifier == kDuration) {
    return v8::Number::New(isolate, duration_);
  }
  if (identifier == kChannelID) {
    return v8::Local<v8::String>::New(isolate,
        gin::StringToV8(isolate, channel_id_));
  }
  if (identifier == kParentalRatings) {
    return v8::Local<v8::Object>::New(isolate, parental_rating_collection_);
  }

  // methods of CbipObjectOipfProgrammeClass
  if (identifier == kMethodToString) {
    return gin::CreateFunctionTemplate(isolate,
      base::Bind(&CbipObjectOipfProgrammeClass::ToString,
      weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kMethodGetSIDescriptors) {
    return gin::CreateFunctionTemplate(isolate,
      base::Bind(&CbipObjectOipfProgrammeClass::GetSIDescriptors,
      weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  return v8::Local<v8::Value>();
}

bool CbipObjectOipfProgrammeClass::SetNamedProperty(v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (!owner_)
    return false;

  if (identifier == kName) {
    name_ =  gin::V8ToString(value);
    return true;
  }
  if (identifier == kProgrammeID) {
    programme_id_ =  gin::V8ToString(value);
    return true;
  }
  if (identifier == kProgrammeIDType) {
    int prog_id = value->IntegerValue();
    if (prog_id <= 2 && prog_id >= 0)
      programme_id_type_ = prog_id;
    else
      isolate->ThrowException(v8::Exception::Error(gin::StringToV8(isolate,
        "The property with the name programme_id_type has an invalid value")));
    return true;
  }
  if (identifier == kDescription) {
    description_ = gin::V8ToString(value);
    return true;
  }
  if (identifier == kLongDescription) {
     long_description_ = gin::V8ToString(value);
     return true;
  }
  if (identifier == kStartTime) {
    int64_t start_time = value->IntegerValue();
    if (start_time >= 0 )
      start_time_ = start_time;
    else
      isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "The property with the name start_time has negative value")));
    return true;
  }
  if (identifier == kDuration) {
    int64_t duration = value->IntegerValue();
    if (duration >= 0 )
      duration_ = duration;
    else
      isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
          isolate, "Properties with the name duration has negative value")));
    return true;
  }
  if (identifier == kChannelID) {
    channel_id_ =  gin::V8ToString(value);
    return true;
  }
  if (identifier == kParentalRatings) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Properties with the name ParentalRatings is read only")));
    return true;
  }

  // The CbipObjectOipfProgrammeClass has only constants and readonly
  // properties as members

  DCHECK(isolate == owner_->GetIsolate());

  return false;
}

std::vector<std::string> CbipObjectOipfProgrammeClass::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kID_TVA_CRID);
  result.push_back(kID_DVB_EVENT);
  result.push_back(kID_TVA_GROUP_CRID);
  result.push_back(kName);
  result.push_back(kProgrammeID);
  result.push_back(kProgrammeIDType);
  result.push_back(kDescription);
  result.push_back(kLongDescription);
  result.push_back(kStartTime);
  result.push_back(kDuration);
  result.push_back(kChannelID);
  result.push_back(kParentalRatings);
  result.push_back(kMethodGetSIDescriptors);
  // kMethodToString isn't enumerable
  return result;
}

void CbipObjectOipfProgrammeClass::OwnerDeleted() {
  owner_ = 0;
}

void CbipObjectOipfProgrammeClass::ToString(gin::Arguments* args) {
  std::string str_class_name("[object Programme]");
  v8::Handle<v8::Value> result = v8::String::NewFromUtf8(args->isolate(),
      str_class_name.c_str(),
      v8::String::kNormalString,
      str_class_name.length());
  args->Return(result);
}

void CbipObjectOipfProgrammeClass::GetSIDescriptors(gin::Arguments* args) {
  CHECK(owner_);
  v8::Isolate* isolate  = owner_->instance()->GetIsolate();
  v8::Handle<v8::Value> arg1;
  if (!args->GetNext(&arg1)) {
    VLOG(X_LOG_V) << "CbipObjectOipfProgrammeClass::GetSIDescriptors() "
                  << "without 1st argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }
  if (!(arg1->IsInt32() || arg1->IsUint32())) {
    VLOG(X_LOG_V) << "CbipObjectOipfProgrammeClass::GetSIDescriptors() "
                  << "with wrong type for 1st argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }
  v8::Handle<v8::Integer> js_descriptor_tag =
      v8::Handle<v8::Integer>::Cast(arg1->ToInteger());
  const int descriptor_tag = js_descriptor_tag->Value();

  v8::Handle<v8::Value> arg2;
  if (!args->GetNext(&arg2)) {
    VLOG(X_LOG_V) << "CbipObjectOipfProgrammeClass::GetSIDescriptors() "
                  << "without 2nd argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }
  if (!(arg2->IsInt32() || arg2->IsUint32())) {
    VLOG(X_LOG_V) << "CbipObjectOipfProgrammeClass::GetSIDescriptors() "
                  << " with wrong type for 2nd argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }
  v8::Handle<v8::Integer> js_descriptor_tag_extension =
      v8::Handle<v8::Integer>::Cast(arg2->ToInteger());
  const int descriptor_tag_extension = js_descriptor_tag_extension->Value();

  v8::Handle<v8::Value> arg3;
  if (!args->GetNext(&arg3)) {
    VLOG(X_LOG_V) << "CbipObjectOipfProgrammeClass::GetSIDescriptors() "
                  << "without 3rd argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }
  if (!(arg3->IsInt32() || arg3->IsUint32())) {
    VLOG(X_LOG_V) << "CbipObjectOipfProgrammeClass::GetSIDescriptors()"
                  << "with wrong type for 3rd argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }

  v8::Handle<v8::Integer> js_private_data_specifier =
      v8::Handle<v8::Integer>::Cast(arg3->ToInteger());
  const int private_data_specifier = js_private_data_specifier->Value();
  bool is_success;
  int routing_id = owner_->instance()->render_frame()->GetRoutingID();
  peer::oipf::StringCollectionData programme_string_collection;
  owner_->ipc()->GetProgrammeSIDescriptors(
      routing_id, programme_id_,
      descriptor_tag, descriptor_tag_extension,
      private_data_specifier, &is_success,
      &programme_string_collection);
  if (is_success) {
    CbipObjectOipfStringCollectionClass* ptr_string_collection =
        new CbipObjectOipfStringCollectionClass(this,
            programme_string_collection);
    v8::Persistent<v8::Object> js_string_collection;
    js_string_collection.Reset(isolate,
                               ptr_string_collection->GetWrapper(isolate));
    args->Return(v8::Local<v8::Object>::New(isolate, js_string_collection));
    return;
  }
  args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
}

gin::ObjectTemplateBuilder
    CbipObjectOipfProgrammeClass::GetObjectTemplateBuilder(
        v8::Isolate* isolate) {
  return Wrappable<CbipObjectOipfProgrammeClass>::
      GetObjectTemplateBuilder(isolate).AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor*
    CbipObjectOipfProgrammeClass::asGinNamedPropertyInterceptor() {
  return this;
}

PepperPluginInstanceImpl* CbipObjectOipfProgrammeClass::instance() {
  if (owner_)
    return owner_->instance();
  return nullptr;
}

CbipPluginIPC* CbipObjectOipfProgrammeClass::ipc() {
  if (owner_)
    return owner_->ipc();
  return nullptr;
}

v8::Isolate* CbipObjectOipfProgrammeClass::GetIsolate() {
  if (owner_)
    return owner_->GetIsolate();
  return nullptr;
}

}  // namespace content

