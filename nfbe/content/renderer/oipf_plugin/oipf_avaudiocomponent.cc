// Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/oipf_plugin/oipf_avaudiocomponent.h"

#include "content/renderer/oipf_plugin/oipf_video_plugin_object.h"
#include "gin/object_template_builder.h"

#define X_LOG_V 0

namespace {

const char kAudioChannels[] = "audioChannels";
const char kAudioDescription[] = "audioDescription";
const char kComponentTag[] = "componentTag";
const char kEncoding[] = "encoding";
const char kEncrypted[] = "encrypted";
const char kLanguage[] = "language";
const char kPid[] = "pid";
const char kType[] = "type";

const char kToString[] = "toString";

}  // namespace

namespace content {

// static
gin::WrapperInfo OipfAvAudioComponent::kWrapperInfo =
    {gin::kEmbedderNativeGin};

// static
OipfAvAudioComponent* OipfAvAudioComponent::FromV8Object(
    v8::Isolate* isolate, v8::Handle<v8::Object> v8_object) {
  OipfAvAudioComponent* object;
  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &object)) {
    return object;
  }
  return nullptr;
}

OipfAvAudioComponent::OipfAvAudioComponent(v8::Isolate* isolate,
    int componentTag, int pid, int type, const std::string& encoding,
    bool encrypted, std::string language, bool audioDescription,
    int audioChannels)
    : OipfAvComponent(componentTag, pid, type, encoding, encrypted),
      gin::NamedPropertyInterceptor(isolate, this),
      language_(language),
      audio_description_(audioDescription),
      audio_channels_(audioChannels),
      avcontrol_(nullptr),
      avcontrol_index_(-1),
      avcontrol_id_(-1),
      weak_ptr_factory_(this) {
}

OipfAvAudioComponent::~OipfAvAudioComponent() {
  VLOG(X_LOG_V) << "ZZZZ OipfAvAudioComponent::dtor";
}

bool OipfAvAudioComponent::isAudio() {
  return true;
}

OipfAvAudioComponent* OipfAvAudioComponent::toAudio() {
  return this;
}

void OipfAvAudioComponent::setAvControl(OipfVideoPluginObject* avcontrol,
    int avcontrol_index, int avcontrol_id) {
  avcontrol_ = avcontrol;
  avcontrol_index_ = avcontrol_index;
  avcontrol_id_ = avcontrol_id;
}

bool OipfAvAudioComponent::getAvControlInfo(
    OipfAvComponent::AVControlInfo& info) {
  if (!avcontrol_) {
    // shutdown() occurred, or this does not come from AV control.
    return false;
  }
  info.avcontrol = avcontrol_;
  info.avcontrol_index = avcontrol_index_;
  info.avcontrol_id = avcontrol_id_;
  return true;
}

void OipfAvAudioComponent::shutdown() {
  VLOG(X_LOG_V) << "ZZZZ OipfAvAudioComponent::shutdown";
  avcontrol_ = nullptr;
}

v8::Local<v8::Value> OipfAvAudioComponent::GetNamedProperty(
    v8::Isolate* isolate, const std::string& identifier) {
  VLOG(X_LOG_V) << "OipfAvAudioComponent::GetNamedProperty: " << identifier;
  if (identifier == kAudioChannels) {
    return v8::Integer::New(isolate, audio_channels_);
  }
  if (identifier == kAudioDescription) {
    return audio_description_ ? v8::True(isolate) : v8::False(isolate);
  }
  if (identifier == kComponentTag) {
    return v8::Integer::New(isolate, component_tag_);
  }
  if (identifier == kEncoding) {
    v8::MaybeLocal<v8::String> s = v8::String::NewFromUtf8(
        isolate, encoding_.c_str(), v8::String::kNormalString);
    v8::Local<v8::String> r;
    bool b = s.ToLocal(&r);
    if (!b)
      return v8::Undefined(isolate);
    return r;
  }
  if (identifier == kEncrypted) {
    if (encrypted_)
      return v8::True(isolate);
    return v8::False(isolate);
  }
  if (identifier == kLanguage) {
    if (language_.empty())
      return v8::String::Empty(isolate);
    v8::MaybeLocal<v8::String> s = v8::String::NewFromUtf8(
        isolate, language_.c_str(), v8::String::kNormalString);
    v8::Local<v8::String> r;
    bool b = s.ToLocal(&r);
    if (!b)
      return v8::String::Empty(isolate);
    return r;
  }
  if (identifier == kPid) {
    return v8::Integer::New(isolate, pid_);
  }
  if (identifier == kType) {
    DCHECK(type_ == OipfAvComponent::kTypeAudio);
    return v8::Number::New(isolate, OipfAvComponent::kTypeAudio);
  }
  if (identifier == kToString) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfAvAudioComponent::ToString,
    weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  return v8::Local<v8::Value>();
}

bool OipfAvAudioComponent::SetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (identifier == kAudioChannels) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name audioChannels")));
    return true;
  }
  if (identifier == kAudioDescription) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name audioDescription")));
    return true;
  }
  if (identifier == kComponentTag) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name componentTag")));
    return true;
  }
  if (identifier == kEncoding) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name encoding")));
    return true;
  }
  if (identifier == kEncrypted) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name encrypted")));
    return true;
  }
  if (identifier == kLanguage) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name language")));
    return true;
  }
  if (identifier == kPid) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name pid")));
    return true;
  }
  if (identifier == kType) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name type")));
    return true;
  }
  return false;
}

std::vector<std::string> OipfAvAudioComponent::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kAudioChannels);
  result.push_back(kAudioDescription);
  result.push_back(kComponentTag);
  result.push_back(kEncoding);
  result.push_back(kEncrypted);
  result.push_back(kLanguage);
  result.push_back(kPid);
  result.push_back(kType);
  return result;
}

gin::ObjectTemplateBuilder OipfAvAudioComponent::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<OipfAvAudioComponent>::GetObjectTemplateBuilder(isolate)
      .AddNamedPropertyInterceptor();
}

void OipfAvAudioComponent::ToString(gin::Arguments* args) {
  std::string str_class_name("[object AVAudioComponent]");
  v8::Handle<v8::Value> result =
      v8::String::NewFromUtf8(args->isolate(),
                              str_class_name.c_str(),
                              v8::String::kNormalString,
                              str_class_name.length());
  args->Return(result);
}

void OipfAvAudioComponent::GetComponentsData(
    peer::oipf::AVComponentsData *av_components_data) {
  av_components_data->component_tag = component_tag_;
  av_components_data->pid = pid_;
  av_components_data->encoding = encoding_;
  av_components_data->encrypted = encrypted_;
  av_components_data->type = type_;
  av_components_data->audio_language = language_;
  av_components_data->audio_description = audio_description_;
  av_components_data->audio_channels = audio_channels_;
}

}  // namespace content
