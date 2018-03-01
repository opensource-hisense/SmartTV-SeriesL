#include "content/renderer/oipf_plugin/oipf_avcomponentcollection.h"

#include "components/plugins/renderer/oipf_video_plugin.h"
#include "content/renderer/oipf_plugin/oipf_avcomponent.h"
#include "content/renderer/oipf_plugin/oipf_avvideocomponent.h"
#include "gin/object_template_builder.h"

#define X_LOG_V 0

namespace {

const char kItem[] = "item";
const char kLength[] = "length";

const char kToString[] = "toString";
}  // namespace

namespace content {

// static
gin::WrapperInfo OipfAvComponentCollection::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
OipfAvComponentCollection* OipfAvComponentCollection::FromV8Object(
    v8::Isolate* isolate, v8::Handle<v8::Object> v8_object) {
  OipfAvComponentCollection* object;
  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &object)) {
    return object;
  }
  return nullptr;
}

OipfAvComponentCollection::OipfAvComponentCollection(
    v8::Isolate* isolate,
    std::vector<OipfAvComponent::AvComponentHandle> components)
    : gin::NamedPropertyInterceptor(isolate, this),
      gin::IndexedPropertyInterceptor(isolate, this),
      components_(components),
      weak_ptr_factory_(this) {
}

OipfAvComponentCollection::~OipfAvComponentCollection() {
  VLOG(X_LOG_V) << "ZZZZ OipfAvComponentCollection::dtor:"
    ;
}

gin::ObjectTemplateBuilder OipfAvComponentCollection::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<OipfAvComponentCollection>::GetObjectTemplateBuilder(isolate)
    .AddNamedPropertyInterceptor().AddIndexedPropertyInterceptor();
}

v8::Local<v8::Value> OipfAvComponentCollection::GetNamedProperty(
    v8::Isolate* isolate, const std::string& identifier) {
  if (identifier == kItem) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfAvComponentCollection::Item,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kLength) {
    return v8::Integer::New(isolate, components_.size());
  }
  if (identifier == kToString) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfAvComponentCollection::ToString,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  return v8::Local<v8::Value>();
}

bool OipfAvComponentCollection::SetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (identifier == kItem) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name item")));
    return true;
  }
  if (identifier == kLength) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name length")));
    return true;
  }
  return false;
}

std::vector<std::string> OipfAvComponentCollection::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kItem);
  result.push_back(kLength);
  return result;
}

v8::Local<v8::Value> OipfAvComponentCollection::GetIndexedProperty(
    v8::Isolate* isolate, uint32_t index) {
  if (index < components_.size()) {
    OipfAvComponent::AvComponentHandle v_ph = components_[index];
    if (!v_ph.IsEmpty()) {
      return v8::Local<v8::Object>::New(isolate, v_ph);
    }
  }
  return v8::Undefined(isolate);
}

bool OipfAvComponentCollection::SetIndexedProperty(
    v8::Isolate* isolate, uint32_t index, v8::Local<v8::Value> value) {
  isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
      isolate, "Cannot set anonymous properties with index")));
  return true;
}

std::vector<uint32_t> OipfAvComponentCollection::EnumerateIndexedProperties(
    v8::Isolate* isolate) {
  std::vector<uint32_t> result;
  for (uint32_t index = 0; index < components_.size(); index += 1)
    result.push_back(index);
  return result;
}

void OipfAvComponentCollection::Item(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();

  v8::Handle<v8::Value> v_tmp;
  double p0_d;

  if (!args->GetNext(&v_tmp)) {
    args->ThrowError();
    return;
  }
  if (!v_tmp->IsNumber() && !v_tmp->IsNumberObject()) {
    args->ThrowError();
    return;
  }
  p0_d = v_tmp->ToNumber(isolate)->Value();

  int index = (int)p0_d;
  if ((double)index != p0_d) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  if (index < 0 || components_.size() <= index) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  args->Return(v8::Local<v8::Object>::New(isolate, components_[index]));
}

void OipfAvComponentCollection::ToString(gin::Arguments* args) {
  std::string str_class_name("[object AVComponentCollection]");
  v8::Handle<v8::Value> result = v8::String::NewFromUtf8(args->isolate(), str_class_name.c_str(), v8::String::kNormalString, str_class_name.length());
  args->Return(result);
}

}  // namespace content

