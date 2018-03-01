// Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_object_oipf_keyset.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_application_private_data.h"
#include "content/renderer/render_thread_impl.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "gin/public/gin_embedders.h"
#include "v8/include/v8.h"

#define X_LOG_V 0

namespace content {

namespace {

const char kALPHA[] = "ALPHA";
const char kBLUE[] = "BLUE";
const char kGREEN[] = "GREEN";
const char kINFO[] = "INFO";
const char kNAVIGATION[] = "NAVIGATION";
const char kNUMERIC[] = "NUMERIC";
const char kOTHER[] = "OTHER";
const char kRED[] = "RED";
const char kSCROLL[] = "SCROLL";
const char kVCR[] = "VCR";
const char kYELLOW[] = "YELLOW";
const char kSetValue[] = "setValue";
const char kValue[] = "value";
const char kOtherKeys[] = "otherKeys";
const char kMaximumValue[] = "maximumValue";
const char kMaximumOtherKeys[] = "maximumOtherKeys";

// Keyset class method.
const char kMethodToString[] = "toString";

}  // namespace

// only ApplicationPrivateData should call this function.
CbipObjectOipfKeyset::CbipObjectOipfKeyset(
    CbipObjectOipfApplicationPrivateData* owner)
    : gin::NamedPropertyInterceptor(owner->GetIsolate(), this),
      owner_(owner),
      weak_ptr_factory_(this) {
}

CbipObjectOipfKeyset::~CbipObjectOipfKeyset() {
  {
    VLOG(X_LOG_V) << "ZZZZ: CbipObjectOipfKeyset::dtor:"
                  << " this=" << static_cast<void*>(this);
  }
}

// static
gin::WrapperInfo CbipObjectOipfKeyset::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
CbipObjectOipfKeyset* CbipObjectOipfKeyset::FromV8Object(
    v8::Isolate* isolate,
    v8::Handle<v8::Object> v8_object) {
  CbipObjectOipfKeyset* self;
  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &self)) {
    return self;
  }
  return NULL;
}

v8::Local<v8::Value> CbipObjectOipfKeyset::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  {
    VLOG(X_LOG_V) << "ZZZZ CbipObjectOipfKeyset::GetNamedProperty:"
                  << " " << identifier;
  }

  if (!owner_)
    return v8::Local<v8::Value>();

  DCHECK(isolate == owner_->GetIsolate());

  if (identifier == kALPHA) {
    return v8::Integer::New(isolate, 0x200);
  }
  if (identifier == kBLUE) {
    return v8::Integer::New(isolate, 0x8);
  }
  if (identifier == kGREEN) {
    return v8::Integer::New(isolate, 0x2);
  }
  if (identifier == kINFO) {
    return v8::Integer::New(isolate, 0x80);
  }
  if (identifier == kNAVIGATION) {
    return v8::Integer::New(isolate, 0x10);
  }
  if (identifier == kNUMERIC) {
    return v8::Integer::New(isolate, 0x100);
  }
  if (identifier == kOTHER) {
    return v8::Integer::New(isolate, 0x400);
  }
  if (identifier == kRED) {
    return v8::Integer::New(isolate, 0x1);
  }
  if (identifier == kSCROLL) {
    return v8::Integer::New(isolate, 0x40);
  }
  if (identifier == kVCR) {
    return v8::Integer::New(isolate, 0x20);
  }
  if (identifier == kYELLOW) {
    return v8::Integer::New(isolate, 0x4);
  }
  if (identifier == kSetValue) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipObjectOipfKeyset::setValue,
            weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kValue) {
    return v8::Integer::New(isolate, value_);
  }
  if (identifier == kOtherKeys) {
    // currently we do not support other keys.
    // FIXME: ask the browser for the updated set of other keys that
    // the JS application has requested, and fill this array.
    return v8::Array::New(isolate, 0);
  }
  if (identifier == kMaximumValue) {
    // currently this is a fixed set.
    // FIXME: ask the browser for the supported intrinsic keys.
    return v8::Integer::New(isolate, 0x3ff);
  }
  if (identifier == kMaximumOtherKeys) {
    // currently we do not support other keys.
    // FIXME: ask the browser for the supported other keys.
    return v8::Array::New(isolate, 0);
  }
  if (identifier == kMethodToString) {
    return gin::CreateFunctionTemplate(isolate,
       base::Bind(&CbipObjectOipfKeyset::ToString,
           weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  return v8::Local<v8::Value>();
}

bool CbipObjectOipfKeyset::SetNamedProperty(v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (!owner_)
    return false;

  DCHECK(isolate == owner_->GetIsolate());

  if (identifier == kALPHA) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name ALPHA")));
    return true;
  }
  if (identifier == kBLUE) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name BLUE")));
    return true;
  }
  if (identifier == kGREEN) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name GREEN")));
    return true;
  }
  if (identifier == kINFO) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name INFO")));
    return true;
  }
  if (identifier == kNAVIGATION) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name NAVIGATION")));
    return true;
  }
  if (identifier == kNUMERIC) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name NUMERIC")));
    return true;
  }
  if (identifier == kOTHER) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name OTHER")));
    return true;
  }
  if (identifier == kRED) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name RED")));
    return true;
  }
  if (identifier == kSCROLL) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name SCROLL")));
    return true;
  }
  if (identifier == kVCR) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name VCR")));
    return true;
  }
  if (identifier == kYELLOW) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name YELLOW")));
    return true;
  }
  if (identifier == kSetValue) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name setValue")));
    return true;
  }

  return false;
}

std::vector<std::string> CbipObjectOipfKeyset::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kALPHA);
  result.push_back(kBLUE);
  result.push_back(kGREEN);
  result.push_back(kINFO);
  result.push_back(kNAVIGATION);
  result.push_back(kNUMERIC);
  result.push_back(kOTHER);
  result.push_back(kRED);
  result.push_back(kSCROLL);
  result.push_back(kVCR);
  result.push_back(kYELLOW);
  result.push_back(kSetValue);
  // kMethodToString isn't enumerable.
  return result;
}

void CbipObjectOipfKeyset::ToString(gin::Arguments* args) {
  std::string str_class_name("[object Keyset]");
  v8::Handle<v8::Value> result =
      v8::String::NewFromUtf8(args->isolate(),
                              str_class_name.c_str(),
                              v8::String::kNormalString,
                              str_class_name.length());
  args->Return(result);
}

void CbipObjectOipfKeyset::OwnerDeleted() {
  owner_ = 0;
}

gin::ObjectTemplateBuilder CbipObjectOipfKeyset::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<CbipObjectOipfKeyset>::GetObjectTemplateBuilder(isolate)
      .AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor*
    CbipObjectOipfKeyset::asGinNamedPropertyInterceptor() {
  return this;
}

void CbipObjectOipfKeyset::setValue(gin::Arguments* args) {
  {
    VLOG(X_LOG_V) << "######## OipfKeyset::setValue: ENTER";
  }

  if (!owner_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  v8::Isolate* isolate = owner_->GetIsolate();
  DCHECK(isolate == args->isolate());

  v8::Handle<v8::Value> v_tmp;
  v8::Local<v8::Value> result;

  // do not really check if |value| is an integer value.
  if (!args->GetNext(&v_tmp)) {
    VLOG(X_LOG_V) << "######## OipfKeyset::setValue:"
                  << " error at " << __LINE__;
    return;
  }
  if (!(v_tmp->IsInt32() || v_tmp->IsUint32())) {
    VLOG(X_LOG_V) << "######## OipfKeyset::setValue:"
                  << " error at " << __LINE__;
    return;
  }
  int value = v8::Handle<v8::Integer>::Cast(v_tmp->ToInteger())->Value();

  // if there is a second argument, it must be a v8::Array.
  if (args->GetNext(&v_tmp)) {
    if (!v_tmp->IsArray()) {
      VLOG(X_LOG_V) << "######## OipfKeyset::setValue:"
                    << " error at " << __LINE__;
      return;
    }
    // do not really check if all the elements are integer values.
  }

  // FIXME: implement a blocking IPC to tell the browser the keyset
  // this Application object wants to receive, and receive the keyset
  // that the browser actually set.
  value_ = value;

  VLOG(X_LOG_V) << "######## OipfKeyset::setValue: value_=" << value_;

  result = v8::Integer::New(isolate, value_);

  args->Return(result);
}

}  // namespace content
