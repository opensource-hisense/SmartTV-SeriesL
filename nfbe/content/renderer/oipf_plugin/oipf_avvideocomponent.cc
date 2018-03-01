#include "content/renderer/oipf_plugin/oipf_avvideocomponent.h"

#include "content/renderer/oipf_plugin/oipf_video_plugin_object.h"
#include "gin/object_template_builder.h"

#define X_LOG_V 0

namespace {

const char kAspectRatio[] = "aspectRatio";
const char kComponentTag[] = "componentTag";
const char kEncoding[] = "encoding";
const char kEncrypted[] = "encrypted";
const char kPid[] = "pid";
const char kType[] = "type";

const char kToString[] = "toString";

}  // namespace

namespace content {

// static
gin::WrapperInfo OipfAvVideoComponent::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
OipfAvVideoComponent* OipfAvVideoComponent::FromV8Object(
    v8::Isolate* isolate, v8::Handle<v8::Object> v8_object) {
  OipfAvVideoComponent* object;
  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &object)) {
    return object;
  }
  return nullptr;
}

OipfAvVideoComponent::OipfAvVideoComponent(v8::Isolate* isolate,
    int componentTag, int pid, int type, const std::string& encoding,
    bool encrypted, double aspectRatio)
    : OipfAvComponent(componentTag, pid, type, encoding, encrypted),
      gin::NamedPropertyInterceptor(isolate, this),
      aspect_ratio_(aspectRatio),
      avcontrol_(nullptr),
      avcontrol_index_(-1),
      avcontrol_id_(-1),
      weak_ptr_factory_(this) {
}

OipfAvVideoComponent::~OipfAvVideoComponent() {
  VLOG(X_LOG_V) << "ZZZZ OipfAvVideoComponent::dtor:"
    ;
}

bool OipfAvVideoComponent::isVideo() {
  return true;
}

OipfAvVideoComponent* OipfAvVideoComponent::toVideo() {
  return this;
}

void OipfAvVideoComponent::setAvControl(OipfVideoPluginObject* avcontrol,
    int avcontrol_index, int avcontrol_id) {
  avcontrol_ = avcontrol;
  avcontrol_index_ = avcontrol_index;
  avcontrol_id_ = avcontrol_id;
}

bool OipfAvVideoComponent::getAvControlInfo(
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

void OipfAvVideoComponent::shutdown() {
  VLOG(X_LOG_V) << "ZZZZ OipfAvVideoComponent::shutdown:"
    ;
  avcontrol_ = nullptr;
}

v8::Local<v8::Value> OipfAvVideoComponent::GetNamedProperty(
    v8::Isolate* isolate, const std::string& identifier) {

  VLOG(X_LOG_V) << "ZZZZ OipfAvVideoComponent::GetNamedProperty: " << identifier;

  if (identifier == kAspectRatio) {
    double r = aspect_ratio_;
    if (r == r) {
      return v8::Number::New(isolate, r);
    }
    // FIXME: we must delay our 'onTrackAdded' events until after
    // 'loadedmetadata' is handled.  the reason is, the spec says that,
    // when the duration of the media is decided, video width and height are
    // set, and 'loadedmetadata' event fires.  i.e. when 'addtrack' fires,
    // video width and height are *not* set.  'addtrack' is handled later,
    // but those values are usually still not set because the duration is
    // not available.  in such case, |aspectRatio_| is initialized to NaN.
    // the below is a workaround for this situation.  now the problem is,
    // in such case, if the application tries to obtain 'aspectRatio'
    // property after the AV control object is destoyed, the once-known
    // valid value is already lost and NaN is returned.
    if (avcontrol_) {
      r = avcontrol_->GetAspectRatio(this, avcontrol_index_, avcontrol_id_);
      if (r == r) {
        aspect_ratio_ = r;
        return v8::Number::New(isolate, r);
      }
    }
    // [OIPFDAE] 7.16.5.3.1
    // "... or undefined if the aspect ratio is not known."
    return v8::Undefined(isolate);
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
  if (identifier == kPid) {
    return v8::Integer::New(isolate, pid_);
  }
  if (identifier == kType) {
    DCHECK(type_ == OipfAvComponent::kTypeVideo);
    return v8::Number::New(isolate, OipfAvComponent::kTypeVideo);
  }
  if (identifier == kToString) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfAvVideoComponent::ToString,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  return v8::Local<v8::Value>();
}

bool OipfAvVideoComponent::SetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {

  if (false)
  {
    VLOG(X_LOG_V) << "ZZZZ OipfAvVideoComponent::SetNamedProperty:"
                  << " " << identifier
      ;
  }

  if (identifier == kAspectRatio) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name aspectRatio")));
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

std::vector<std::string> OipfAvVideoComponent::EnumerateNamedProperties(
    v8::Isolate* isolate) {

  if (false)
  {
    VLOG(X_LOG_V) << "ZZZZ OipfAvVideoComponent::EnumerateNamedProperties:"
      ;
  }

  std::vector<std::string> result;
  result.push_back(kAspectRatio);
  result.push_back(kComponentTag);
  result.push_back(kEncoding);
  result.push_back(kEncrypted);
  result.push_back(kPid);
  result.push_back(kType);
  return result;
}

gin::ObjectTemplateBuilder OipfAvVideoComponent::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<OipfAvVideoComponent>::GetObjectTemplateBuilder(isolate)
      .AddNamedPropertyInterceptor();
}

void OipfAvVideoComponent::ToString(gin::Arguments* args) {
  std::string str_class_name("[object AVVideoComponent]");
  v8::Handle<v8::Value> result = v8::String::NewFromUtf8(args->isolate(), str_class_name.c_str(), v8::String::kNormalString, str_class_name.length());
  args->Return(result);
}

void OipfAvVideoComponent::GetComponentsData(peer::oipf::AVComponentsData *av_components_data) {
  av_components_data->component_tag = component_tag_;
  av_components_data->pid = pid_;
  av_components_data->encoding = encoding_;
  av_components_data->encrypted = encrypted_;
  av_components_data->type = type_;
  av_components_data->aspect_ratio = aspect_ratio_;
}

}  // namespace content
