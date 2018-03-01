//Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/oipf_plugin/oipf_video_plugin_object.h"

#include "base/bind.h"
#include "base/logging.h"
#include "components/plugins/renderer/oipf_video_plugin.h"
#include "content/renderer/oipf_plugin/oipf_avaudiocomponent.h"
#include "content/renderer/oipf_plugin/oipf_avcomponent.h"
#include "content/renderer/oipf_plugin/oipf_avcomponentcollection.h"
#include "content/renderer/oipf_plugin/oipf_avvideocomponent.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "gin/public/gin_embedders.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "v8/include/v8.h"

#define X_LOG_V 0

namespace content {

namespace {

const char kError[] = "error";
const char kGetComponents[] = "getComponents";
const char kGetCurrentActiveComponents[] = "getCurrentActiveComponents";
const char kPlay[] = "play";
const char kPlayPosition[] = "playPosition";
const char kPlayState[] = "playState";
const char kSeek[] = "seek";
const char kSelectComponent[] = "selectComponent";
const char kSetFullScreen[] = "setFullScreen";
const char kSpeed[] = "speed";
const char kStop[] = "stop";
const char kUnselectComponent[] = "unselectComponent";

}  // namespace

OipfVideoPluginObject::OipfVideoPluginObject(plugins::OipfVideoPlugin* owner,
    v8::Isolate* isolate)
    : gin::NamedPropertyInterceptor(isolate, this),
      owner_(owner),
      isolate_(isolate),
      weak_ptr_factory_(this) {
}

OipfVideoPluginObject::~OipfVideoPluginObject() {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::dtor:";
  DCHECK(!owner_);
}

void OipfVideoPluginObject::shutdown() {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::shutdown:";
  DCHECK(owner_);
  v8::Isolate* isolate = isolate_;
  DCHECK(owner_->isolate_r() == isolate);
  std::vector<OipfAvComponent::AvComponentHandle> components_copy;
  components_.swap(components_copy);
  for (std::vector<OipfAvComponent::AvComponentHandle>::iterator it =
           components_copy.begin();
       it != components_copy.end();
       ++it) {
    OipfAvVideoComponent* v = OipfAvVideoComponent::FromV8Object(
        isolate, v8::Local<v8::Object>::New(isolate, *it));
    if (v) {
      v->shutdown();
      continue;
    }
    OipfAvAudioComponent* a = OipfAvAudioComponent::FromV8Object(
        isolate, v8::Local<v8::Object>::New(isolate, *it));
    if (a) {
      a->shutdown();
      continue;
    }
    DCHECK(0);
  }
  owner_ = nullptr;
}

// static
gin::WrapperInfo OipfVideoPluginObject::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
OipfVideoPluginObject* OipfVideoPluginObject::FromV8Object(
    v8::Isolate* isolate, v8::Handle<v8::Object> v8_object) {
  OipfVideoPluginObject* object;
  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &object)) {
    return object;
  }
  return nullptr;
}

v8::Local<v8::Value> OipfVideoPluginObject::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  DCHECK( isolate == isolate_ );

  if (identifier == kError) {
    int v = GetError(isolate);
    return v8::Integer::New(isolate, v);
  }
  if (identifier == kGetComponents) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPluginObject::GetComponents,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kGetCurrentActiveComponents) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPluginObject::GetCurrentActiveComponents,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kPlay) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPluginObject::Play,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kPlayPosition) {
    double v = GetPlayPosition(isolate);
    return v8::Number::New(isolate, v);
  }
  if (identifier == kPlayState) {
    int v = GetPlayState(isolate);
    return v8::Integer::New(isolate, v);
  }
  if (identifier == kSeek) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPluginObject::Seek,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kSelectComponent) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPluginObject::SelectComponent,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kSetFullScreen) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPluginObject::SetFullScreen,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kSpeed) {
    double v = GetSpeed(isolate);
    return v8::Number::New(isolate, v);
  }
  if (identifier == kStop) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPluginObject::Stop,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kUnselectComponent) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPluginObject::UnselectComponent,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  return v8::Local<v8::Value>();
}

bool OipfVideoPluginObject::SetNamedProperty(v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {

  if (identifier == kError) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name error")));
    return true;
  }
  if (identifier == kGetCurrentActiveComponents) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate,
        "Cannot set properties with the name getCurrentActiveComponents")));
    return true;
  }
  if (identifier == kGetComponents) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name getComponents")));
    return true;
  }
  if (identifier == kPlay) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name play")));
    return true;
  }
  if (identifier == kPlayPosition) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name playPosition")));
    return true;
  }
  if (identifier == kPlayState) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name playState")));
    return true;
  }
  if (identifier == kSeek) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name seek")));
    return true;
  }
  if (identifier == kSelectComponent) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name selectComponent")));
    return true;
  }
  if (identifier == kSetFullScreen) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name setFullScreen")));
    return true;
  }
  if (identifier == kSpeed) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name speed")));
    return true;
  }
  if (identifier == kStop) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name stop")));
    return true;
  }
  if (identifier == kUnselectComponent) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name unselectComponent")));
    return true;
  }

  return false;
}

std::vector<std::string> OipfVideoPluginObject::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;

  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::EnumerateNamedProperties:";

  result.push_back(kError);
  result.push_back(kGetComponents);
  result.push_back(kGetCurrentActiveComponents);
  result.push_back(kPlay);
  result.push_back(kPlayPosition);
  result.push_back(kPlayState);
  result.push_back(kSeek);
  result.push_back(kSelectComponent);
  result.push_back(kSetFullScreen);
  result.push_back(kSpeed);
  result.push_back(kStop);
  result.push_back(kUnselectComponent);

  return result;
}

gin::ObjectTemplateBuilder OipfVideoPluginObject::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<OipfVideoPluginObject>::GetObjectTemplateBuilder(isolate)
      .AddNamedPropertyInterceptor();
}

void OipfVideoPluginObject::Play(gin::Arguments* args)
{
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::Play";

  DCHECK(isolate_ == args->isolate());

  if (!owner_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  // inside plugin
  WebViewPlugin* wvp = owner_->plugin();
  if (!wvp) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  blink::WebView* wv = wvp->web_view();
  blink::WebFrame* p_frame = wv->mainFrame();

  // outside plugin
  blink::WebPluginContainer* container = wvp->container();
  blink::WebLocalFrame* r_frame_0 = 0;
  if (container) {
    blink::WebElement r_element = container->element();
    blink::WebDocument r_document = r_element.document();
    r_frame_0 = r_document.frame();
  }
  blink::WebLocalFrame* r_frame_1 = owner_->render_frame()->GetWebFrame();

  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::Play:"
                << " p_frame=" << static_cast<void*>(p_frame)
                << " r_frame_0=" << static_cast<void*>(r_frame_0)
                << " r_frame_1=" << static_cast<void*>(r_frame_1)
    ;

  v8::Local<v8::Object> pobj = owner_->GetWrapper(isolate_);

  v8::Handle<v8::Value> v_tmp;
  double p0_d;

  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  if (!args->GetNext(&v_tmp)) {
    args->ThrowError();
    return;
  }
  if (!v_tmp->IsNumber() && !v_tmp->IsNumberObject()) {
    args->ThrowError();
    return;
  }
  p0_d = v_tmp->ToNumber(isolate_)->Value();

  key = v8::String::NewFromUtf8(isolate_, "play", v8::String::kNormalString, 4);
  prop = pobj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::Play: play is not a function";
    owner_->setInitialPlayState(p0_d);
    args->Return(v8::Local<v8::Value>());
    return;
  }
  function = v8::Handle<v8::Function>::Cast(prop);

  {
    v8::Handle<v8::Value> p0 = v8::Number::New(isolate_, p0_d);
    v8::Handle<v8::Value> argv[] = { p0 };
    v8::Handle<v8::Value> result = p_frame->callFunctionEvenIfScriptDisabled(
        function, pobj, 1, argv);
    (void)result;
  }

  args->Return(v8::Local<v8::Value>());
}

void OipfVideoPluginObject::Stop(gin::Arguments* args)
{
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::Stop";

  DCHECK(isolate_ == args->isolate());

  if (!owner_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  // inside plugin
  WebViewPlugin* wvp = owner_->plugin();
  if (!wvp) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  blink::WebView* wv = wvp->web_view();
  blink::WebFrame* p_frame = wv->mainFrame();

  // outside plugin
  blink::WebPluginContainer* container = wvp->container();
  blink::WebLocalFrame* r_frame_0 = 0;
  if (container) {
    blink::WebElement r_element = container->element();
    blink::WebDocument r_document = r_element.document();
    r_frame_0 = r_document.frame();
  }
  blink::WebLocalFrame* r_frame_1 = owner_->render_frame()->GetWebFrame();

  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::Stop:"
                << " p_frame=" << static_cast<void*>(p_frame)
                << " r_frame_0=" << static_cast<void*>(r_frame_0)
                << " r_frame_1=" << static_cast<void*>(r_frame_1)
    ;

  v8::Local<v8::Object> p_obj = owner_->GetWrapper(isolate_);

  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  key = v8::String::NewFromUtf8(isolate_, "stop", v8::String::kNormalString, 4);
  prop = p_obj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::Stop: stop is not a function";
    args->Return(v8::Local<v8::Value>());
    return;
  }
  function = v8::Handle<v8::Function>::Cast(prop);

  {
    v8::Handle<v8::Value> argv[] = { };
    v8::Handle<v8::Value> result = p_frame->callFunctionEvenIfScriptDisabled(
        function, p_obj, 0, argv);
    (void)result;
  }

  args->Return(v8::Local<v8::Value>());
}

void OipfVideoPluginObject::Seek(gin::Arguments* args)
{
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::Seek";

  DCHECK(isolate_ == args->isolate());

  if (!owner_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  // inside plugin
  WebViewPlugin* wvp = owner_->plugin();
  if (!wvp) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  blink::WebView* wv = wvp->web_view();
  blink::WebFrame* p_frame = wv->mainFrame();

  v8::Local<v8::Object> p_obj = owner_->GetWrapper(isolate_);

  v8::Handle<v8::Value> v_tmp;
  double p0_d;  // seek position in milli-seconda

  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  if (!args->GetNext(&v_tmp)) {
    args->ThrowError();
    return;
  }
  if (!v_tmp->IsNumber() && !v_tmp->IsNumberObject()) {
    args->ThrowError();
    return;
  }
  p0_d = v_tmp->ToNumber(isolate_)->Value();

  key = v8::String::NewFromUtf8(isolate_, "seek", v8::String::kNormalString, 4);
  prop = p_obj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::Seek: seek is not a function";
    args->Return(v8::Local<v8::Value>());
    return;
  }
  function = v8::Handle<v8::Function>::Cast(prop);

  {
    v8::Handle<v8::Value> p0 = v8::Number::New(isolate_, p0_d);
    v8::Handle<v8::Value> argv[] = { p0 };
    v8::Handle<v8::Value> result = p_frame->callFunctionEvenIfScriptDisabled(
        function, p_obj, 1, argv);
    (void)result;
  }

  args->Return(v8::Local<v8::Value>());
}

void OipfVideoPluginObject::SetFullScreen(gin::Arguments* args)
{
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::SetFullScreen:";

  DCHECK(isolate_ == args->isolate());

  if (!owner_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  WebViewPlugin* wvp = owner_->plugin();
  if (!wvp) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  // outside plugin
  blink::WebPluginContainer* container = wvp->container();
  blink::WebLocalFrame* r_frame_0 = 0;
  if (container) {
    blink::WebElement r_element = container->element();
    blink::WebDocument r_document = r_element.document();
    r_frame_0 = r_document.frame();
  }

  blink::WebLocalFrame* r_frame = r_frame_0;
  if (!r_frame) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  v8::Handle<v8::Value> v_tmp;

  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  if (!args->GetNext(&v_tmp)) {
    args->ThrowError();
    return;
  }
  if (!v_tmp->IsBoolean() && !v_tmp->IsBooleanObject()) {
    args->ThrowError();
    return;
  }

  if (v_tmp->IsTrue()) {
    v8::Handle<v8::Object> r_element_obj = container->v8ObjectForElement();
    // modify styles so that this element will stretch after fullscreen.

    //
    key = v8::String::NewFromUtf8(isolate_, "webkitRequestFullscreen",
        v8::String::kNormalString, 23);
    prop = r_element_obj->Get(key);
    if (!prop->IsFunction()) {
      VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::SetFullScreen:"
                    << " webkitRequestFullscreen is not a function";
      args->Return(v8::Local<v8::Value>());
      return;
    }
    function = v8::Handle<v8::Function>::Cast(prop);
    {
      v8::Handle<v8::Value> argv[] = { };
      v8::Handle<v8::Value> result = r_frame->callFunctionEvenIfScriptDisabled(
          function, r_element_obj, 0, argv);
      (void)result;
    }
  } else {
    // FIXME: there is a fullscreen request stack.
    // how can we cleanly remove this element from the stack?
    v8::Local<v8::Object> r_window_obj =
        r_frame->mainWorldScriptContext()->Global();
    key = v8::String::NewFromUtf8(isolate_, "document",
        v8::String::kNormalString, 8);
    prop = r_window_obj->Get(key);
    if (!prop->IsObject()) {
      VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::SetFullScreen:"
                    << " global.document is not an object";
      args->Return(v8::Local<v8::Value>());
      return;
    }
    v8::Handle<v8::Object> r_document_obj = v8::Handle<v8::Object>::Cast(prop);
    key = v8::String::NewFromUtf8(isolate_, "webkitExitFullscreen",
        v8::String::kNormalString, 20);
    prop = r_document_obj->Get(key);
    if (!prop->IsFunction()) {
      VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::SetFullScreen:"
                    << " document.webkitExitFullscreen is not a function";
      args->Return(v8::Local<v8::Value>());
      return;
    }
    function = v8::Handle<v8::Function>::Cast(prop);
    {
      v8::Handle<v8::Value> argv[] = { };
      v8::Handle<v8::Value> result = r_frame->callFunctionEvenIfScriptDisabled(
          function, r_document_obj, 0, argv);
      (void)result;
    }
  }

  args->Return(v8::Local<v8::Value>());
}

void OipfVideoPluginObject::GetComponents(gin::Arguments* args)
{
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::GetComponents:";

  DCHECK(isolate_ == args->isolate());

  if (!owner_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  v8::Isolate* isolate = isolate_;
  DCHECK(owner_->isolate_r() == isolate);

  WebViewPlugin* wvp = owner_->plugin();
  if (!wvp) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  // outside plugin
  blink::WebPluginContainer* container = wvp->container();
  blink::WebLocalFrame* r_frame_0 = 0;
  if (container) {
    blink::WebElement r_element = container->element();
    blink::WebDocument r_document = r_element.document();
    r_frame_0 = r_document.frame();
  }

  blink::WebLocalFrame* r_frame = r_frame_0;
  if (!r_frame) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  v8::Handle<v8::Value> v_tmp;
  bool f_collect_video = false;
  bool f_collect_audio = false;
  bool f_collect_subtitle = false;

  if (args->GetNext(&v_tmp)) {
    if (v_tmp->IsNull() || v_tmp->IsUndefined()) {
      f_collect_video = true;
      f_collect_audio = true;
      f_collect_subtitle = true;
    } else if (v_tmp->IsNumber() || v_tmp->IsNumberObject()) {
      double p0_d = v_tmp->ToNumber(isolate)->Value();
      int v_componentType = (int)p0_d;
      if ((double)v_componentType == p0_d) {
        if (v_componentType == OipfAvComponent::kTypeVideo)
          f_collect_video = true;
        else if (v_componentType == OipfAvComponent::kTypeAudio)
          f_collect_audio = true;
        else if (v_componentType == OipfAvComponent::kTypeSubtitle)
          f_collect_subtitle = true;
      }
    }
  } else {
    f_collect_video = true;
    f_collect_audio = true;
    f_collect_subtitle = true;
  }

  std::vector<OipfAvComponent::AvComponentHandle> new_list;

  if (f_collect_video || f_collect_audio || f_collect_subtitle) {
    for (unsigned i = 0; i < components_.size(); i += 1) {
      bool f_collect = false;
      if (!components_[i].IsEmpty()) {
        v8::Local<v8::Object> lh = v8::Local<v8::Object>::New(
            isolate, components_[i]);
        if (f_collect_video) {
          OipfAvVideoComponent* v = OipfAvVideoComponent::FromV8Object(
              isolate, lh);
          if (v)
            f_collect = true;
        }
        if (!f_collect && f_collect_audio) {
          OipfAvAudioComponent* v = OipfAvAudioComponent::FromV8Object(
              isolate, lh);
          if (v)
            f_collect = true;
        }
        // FIXME: impl: subtitle.
        if (f_collect)
          new_list.push_back(components_[i]);
      }
    }
  }

  OipfAvComponentCollection* result = new OipfAvComponentCollection(
      isolate, new_list);

  args->Return(result->GetWrapper(isolate));
}

void OipfVideoPluginObject::GetCurrentActiveComponents(gin::Arguments* args)
{
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::GetCurrentActiveComponents:";

  DCHECK(isolate_ == args->isolate());

  if (!owner_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  v8::Isolate* isolate = isolate_;
  DCHECK(owner_->isolate_r() == isolate);

  WebViewPlugin* wvp = owner_->plugin();
  if (!wvp) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  // outside plugin
  blink::WebPluginContainer* container = wvp->container();
  blink::WebLocalFrame* r_frame_0 = 0;
  if (container) {
    blink::WebElement r_element = container->element();
    blink::WebDocument r_document = r_element.document();
    r_frame_0 = r_document.frame();
  }

  blink::WebLocalFrame* r_frame = r_frame_0;
  if (!r_frame) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  v8::Handle<v8::Value> v_tmp;
  int v_componentType = -1;

  if (!args->GetNext(&v_tmp)) {
    args->ThrowError();
    return;
  }

  if (v_tmp->IsNumber() || v_tmp->IsNumberObject()) {
    double p0_d = v_tmp->ToNumber(isolate)->Value();
    v_componentType = (int)p0_d;
    if ((double)v_componentType != p0_d) {
      v_componentType = -1;
    }
  }

  std::vector<OipfAvComponent::AvComponentHandle> new_list;

  if (v_componentType == OipfAvComponent::kTypeVideo ||
      v_componentType == OipfAvComponent::kTypeAudio ||
      v_componentType == OipfAvComponent::kTypeSubtitle) {
    for (unsigned i = 0; i < components_.size(); i += 1) {
      bool f_collect = false;
      OipfAvComponent::AVControlInfo avcontrol_info;
      if (!components_[i].IsEmpty()) {
        if (v_componentType == OipfAvComponent::kTypeVideo) {
          v8::Local<v8::Object> lh = v8::Local<v8::Object>::New(
              isolate, components_[i]);
          OipfAvVideoComponent* v = OipfAvVideoComponent::FromV8Object(
              isolate, lh);
          if (v)
            f_collect = v->getAvControlInfo(avcontrol_info);
        } else if (v_componentType == OipfAvComponent::kTypeAudio) {
          v8::Local<v8::Object> lh = v8::Local<v8::Object>::New(
              isolate, components_[i]);
          OipfAvAudioComponent* v = OipfAvAudioComponent::FromV8Object(
              isolate, lh);
          if (v)
            f_collect = v->getAvControlInfo(avcontrol_info);
        } else if (v_componentType == OipfAvComponent::kTypeSubtitle) {
          // FIXME: impl
        }
        if (f_collect) {
          // check if the component still exists and is selected.
          if (avcontrol_info.avcontrol != this) {
            // the application passes an AVComponent obtained from a
            // differnt AV control.
            args->Return(v8::Local<v8::Value>());
            return;
          }
          f_collect = owner_->CheckIfTrackIsSelected(
              this, avcontrol_info.avcontrol_index,
              avcontrol_info.avcontrol_id );
        }
        if (f_collect)
          new_list.push_back(components_[i]);
      }
    }
  }

  OipfAvComponentCollection* result = new OipfAvComponentCollection(
      isolate, new_list);

  args->Return(result->GetWrapper(isolate));
}

void OipfVideoPluginObject::SelectComponent(gin::Arguments* args) {
  SelectComponentInternal(args, true);
}

void OipfVideoPluginObject::UnselectComponent(gin::Arguments* args) {
  SelectComponentInternal(args, false);
}

void OipfVideoPluginObject::SelectComponentInternal(
    gin::Arguments* args, bool select) {
  DCHECK(isolate_ == args->isolate());

  if (!owner_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  v8::Isolate* isolate = isolate_;
  DCHECK(owner_->isolate_r() == isolate);

  WebViewPlugin* wvp = owner_->plugin();
  if (!wvp) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  // outside plugin
  blink::WebPluginContainer* container = wvp->container();
  blink::WebLocalFrame* r_frame_0 = 0;
  if (container) {
    blink::WebElement r_element = container->element();
    blink::WebDocument r_document = r_element.document();
    r_frame_0 = r_document.frame();
  }

  blink::WebLocalFrame* r_frame = r_frame_0;
  if (!r_frame) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  v8::Handle<v8::Value> v_tmp;
  int v_componentType = -1;

  if (!args->GetNext(&v_tmp)) {
    args->ThrowError();
    return;
  }

  if (v_tmp->IsObject() && !v_tmp->IsNumberObject()) {
    // selectComponent( AVComponent component )
    //   select |component|.
    // unselectComponent( AVComponent component )
    //   stop playing |component|.
    //   ? ? ? what should we do if |component| is already unselected ? ? ?
    bool f_is_valid = false;
    OipfAvComponent::AVControlInfo avcontrol_info;
    OipfAvVideoComponent* vc = OipfAvVideoComponent::FromV8Object(
        isolate, v8::Handle<v8::Object>::Cast(v_tmp));
    if (vc) {
      f_is_valid = vc->getAvControlInfo(avcontrol_info);
    } else {
      OipfAvAudioComponent* ac = OipfAvAudioComponent::FromV8Object(
          isolate, v8::Handle<v8::Object>::Cast(v_tmp));
      if (ac) {
        f_is_valid = ac->getAvControlInfo(avcontrol_info);
      } else {
        // FIXME: impl: text track.
      }
    }
    if (f_is_valid) {
      if (avcontrol_info.avcontrol != this) {
        // the application passes an AVComponent obtained from a
        // differnt AV control.
        args->Return(v8::Local<v8::Value>());
        return;
      }
      owner_->SelectTrack(this, avcontrol_info.avcontrol_index,
                          avcontrol_info.avcontrol_id, select);
    }
    args->Return(v8::Local<v8::Value>());
    return;
  }

  if (v_tmp->IsNumber() || v_tmp->IsNumberObject()) {
    // selectComponent( Integer componentType )
    //   select the default component of type |componentType|.
    // unselectComponent( Integer componentType )
    //   stop playing component of type |componentType|.
    double p0_d = v_tmp->ToNumber(isolate)->Value();
    int v_componentType = (int)p0_d;
    if ((double)v_componentType != p0_d) {
      args->Return(v8::Local<v8::Value>());
      return;
    }
    int acs_type;
    if (v_componentType == OipfAvComponent::kTypeVideo) {
      acs_type = plugins::OipfVideoPlugin::kAcsTrackTypeVideo;
    } else if (v_componentType == OipfAvComponent::kTypeAudio) {
      acs_type = plugins::OipfVideoPlugin::kAcsTrackTypeAudio;
    } else if (v_componentType == OipfAvComponent::kTypeSubtitle) {
      // FIXME: impl.
      args->Return(v8::Local<v8::Value>());
      return;
    } else {
      args->Return(v8::Local<v8::Value>());
      return;
    }
    owner_->SelectTrack(this, -1, acs_type, select);
  }

  args->Return(v8::Local<v8::Value>());
}

double OipfVideoPluginObject::GetError(v8::Isolate* isolate) {
  // VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::GetError";

  double result_d = -1;

  if (!owner_) {
    return -1;
  }

  // inside plugin
  WebViewPlugin* wvp = owner_->plugin();
  if (!wvp) {
    return -1;
  }
  blink::WebView* wv = wvp->web_view();
  blink::WebFrame* p_frame = wv->mainFrame();

  v8::Local<v8::Object> p_obj = owner_->GetWrapper(isolate_);

  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  key = v8::String::NewFromUtf8(isolate_, "geterror",
      v8::String::kNormalString, 8);
  prop = p_obj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::GetError:"
                  << " geterror is not a function";
    return -1;
  }
  function = v8::Handle<v8::Function>::Cast(prop);

  {
    v8::Handle<v8::Value> argv[] = { };
    v8::Handle<v8::Value> result = p_frame->callFunctionEvenIfScriptDisabled(
        function, p_obj, 0, argv);
    if (!result->IsNumber() && result->IsNumberObject()) {
      return -1;
    }
    result_d = result->ToNumber(isolate)->Value();
  }

  return result_d;
}

double OipfVideoPluginObject::GetPlayPosition(v8::Isolate* isolate) {
  // VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::GetPlayPosition";

  double result_d = -1;

  if (!owner_) {
    return -1;
  }

  // inside plugin
  WebViewPlugin* wvp = owner_->plugin();
  if (!wvp) {
    return -1;
  }
  blink::WebView* wv = wvp->web_view();
  blink::WebFrame* p_frame = wv->mainFrame();

  v8::Local<v8::Object> p_obj = owner_->GetWrapper(isolate_);

  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  key = v8::String::NewFromUtf8(isolate_, "getplayposition",
      v8::String::kNormalString, 15);
  prop = p_obj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::GetPlayPosition:"
                  << " getplayposition is not a function";
    return -1;
  }
  function = v8::Handle<v8::Function>::Cast(prop);

  {
    v8::Handle<v8::Value> argv[] = { };
    v8::Handle<v8::Value> result = p_frame->callFunctionEvenIfScriptDisabled(
        function, p_obj, 0, argv);
    if (!result->IsNumber() && result->IsNumberObject()) {
      return -1;
    }
    result_d = result->ToNumber(isolate)->Value();
  }

  return result_d;
}

double OipfVideoPluginObject::GetPlayState(v8::Isolate* isolate) {
  // VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::GetPlayState";

  double result_d = 0;

  if (!owner_) {
    return 0;
  }

  // inside plugin
  WebViewPlugin* wvp = owner_->plugin();
  if (!wvp) {
    return 0;
  }
  blink::WebView* wv = wvp->web_view();
  blink::WebFrame* p_frame = wv->mainFrame();

  v8::Local<v8::Object> p_obj = owner_->GetWrapper(isolate_);

  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  key = v8::String::NewFromUtf8(isolate_, "getplaystate",
      v8::String::kNormalString, 12);
  prop = p_obj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::GetPlayState:"
                  << " getplaystate is not a function";
    return 0;
  }
  function = v8::Handle<v8::Function>::Cast(prop);

  {
    v8::Handle<v8::Value> argv[] = { };
    v8::Handle<v8::Value> result = p_frame->callFunctionEvenIfScriptDisabled(
        function, p_obj, 0, argv);
    if (!result->IsNumber() && result->IsNumberObject()) {
      return 0;
    }
    result_d = result->ToNumber(isolate)->Value();
  }

  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::GetPlayState: result="
                << result_d;

  return (int)result_d;
}

double OipfVideoPluginObject::GetSpeed(v8::Isolate* isolate) {
  // VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::GetSpeed";

  double result_d = 0;

  if (!owner_) {
    return 0;
  }

  // inside plugin
  WebViewPlugin* wvp = owner_->plugin();
  if (!wvp) {
    return 0;
  }
  blink::WebView* wv = wvp->web_view();
  blink::WebFrame* p_frame = wv->mainFrame();

  v8::Local<v8::Object> p_obj = owner_->GetWrapper(isolate_);

  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  key = v8::String::NewFromUtf8(isolate_, "getspeed",
      v8::String::kNormalString, 8);
  prop = p_obj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::GetSpeed:"
                  << " getspeed is not a function";
    return 0;
  }
  function = v8::Handle<v8::Function>::Cast(prop);

  {
    v8::Handle<v8::Value> argv[] = { };
    v8::Handle<v8::Value> result = p_frame->callFunctionEvenIfScriptDisabled(
        function, p_obj, 0, argv);
    if (!result->IsNumber() && result->IsNumberObject()) {
      return 0;
    }
    result_d = result->ToNumber(isolate)->Value();
  }

  return result_d;
}

// n.b. for MP4 FF, if the media is encrypted or not, is defined to be
// decided when its metadata becomes available. (via 'encrypted' event,
// and the application has to get the key from the server.)
// this should happen before any 'addtrack' event....
//
void OipfVideoPluginObject::onTrackAdded(int tlist_idx,
                                         int acs_id,
                                         int acs_type) {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::onTrackAdded:"
                << " idx=" << tlist_idx
                << " acs_id=" << acs_id
                << " acs_type=" << acs_type
    ;

  WebViewPlugin* wvp = owner_->plugin();
  if (!wvp)
    return;

  // FIXME: should we correctly support this for encrypted MP4 FF ?
  // [OIPFMF] 3 A/V Media Formats: says this is not defined for MP4 FF.
  bool encrypted = false;

  // outside plugin
  blink::WebPluginContainer* container = wvp->container();
  blink::WebLocalFrame* r_frame_0 = 0;
  if (container) {
    blink::WebElement r_element = container->element();
    blink::WebDocument r_document = r_element.document();
    r_frame_0 = r_document.frame();
  }
  blink::WebLocalFrame* r_frame_1 = owner_->render_frame()->GetWebFrame();
  DCHECK(r_frame_0 == r_frame_1);
  (void)r_frame_0;
  (void)r_frame_1;

  blink::WebLocalFrame* r_frame = r_frame_0;
  if (!r_frame)
    return;

  v8::Isolate* isolate = isolate_;
  DCHECK(owner_->isolate_r() == isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(r_frame->mainWorldScriptContext());

  if (components_.capacity() <= tlist_idx) {
    components_.resize(tlist_idx + 1);
  }
  DCHECK(components_[tlist_idx].IsEmpty());

  if (acs_type == plugins::OipfVideoPlugin::kAcsTrackTypeAudio) {
    // [OIPFMF] 3 A/V Media Formats: for MP4 FF: "trackID"
    // FIXME: impl.
    int pid = 0;
    // n.b. [OIPFMF] 3 A/V Media Formats: the supported formats for MP4 FF are,
    // HEAAC, HEAAC2, HEAAC_MPS, AC3, E-AC3, DTS.  and MPEG1_L2, MPEG1_L2_MPS when PAL.
    // [OIPFDAE] 8.4.2 AVComponent: encoding for MP4 FF: if "mp4a" then "audio/mp4".
    // ? ? ? what to do if MP4 FF has non MPEG4 audio (such as MPEG1_L2.) ? ? ?
    std::string encoding("audio/mp4");
    // [OIPFMF] 3 A/V Media Formats: for MP4 FF, "The contents of the
    // language field in the media header "mdhd" of the track."
    // FIXME: the value is a cryptic 16bit integer and needs decoding.
    // ? ? ? AudioTrack.language is BCP 47.  who converts the value ? ? ?
    // https://tools.ietf.org/html/bcp47
    std::string language("");
    // [OIPFMF] 3 A/V Media Formats: for MP4 FF, "Not defined"
    bool audioDescription = false;
    // [OIPFMF] 3 A/V Media Formats: for MP4 FF, "Not defined".  why....
    // FIXME: HTMLMediaElement has no API to tell this value.
    // n.b. Mozilla's HTMLMediaElement.mozChannels is deprecated.
    int audioChannels = 2;
    OipfAvAudioComponent* track = new OipfAvAudioComponent(
        isolate, 0, pid, OipfAvComponent::kTypeAudio, encoding, encrypted,
        language, audioDescription, audioChannels);
    track->setAvControl(this, tlist_idx, acs_id);
    v8::Local<v8::Object> r_lh = track->GetWrapper(isolate);
    components_[tlist_idx].Reset(isolate, r_lh);
  } else if (acs_type == plugins::OipfVideoPlugin::kAcsTrackTypeVideo) {
    // [OIPFMF] 3 A/V Media Formats: for MP4 FF: "trackID"
    // FIXME: impl.
    int pid = 0;
    // see the comment in oipf_video_plugin.cc.
    double dar;
    if (!owner_)
      dar = 0.0/0.0;
    else
      dar = owner_->GetAspectRatio(this, tlist_idx, acs_id);
    // n.b. [OIPFMF] 3 A/V Media Formats: AVC is the only supported format for MP4 FF.
    // [OIPFDAE] 8.4.2 AVComponent: encoding for MP4 FF: if "avc1" then "video/mp4".
    // here, even if type is not "avc1" (i.e. "avc2". or even "avc3"/"avc4") or not AVC
    // (e.g. SP, ASP, H263, ...), return "video/mp4".
    std::string encoding("video/mp4");
    OipfAvVideoComponent* track = new OipfAvVideoComponent(
        isolate, 0, 0, OipfAvComponent::kTypeVideo, encoding, encrypted,
        dar);
    track->setAvControl(this, tlist_idx, acs_id);
    v8::Local<v8::Object> r_lh = track->GetWrapper(isolate);
    components_[tlist_idx].Reset(isolate, r_lh);
  }
}

// n.b. this function is not verified because 'removetrack' event
// never fires when MP4 FF.
void OipfVideoPluginObject::onTrackRemoved(int tlist_idx,
                                           int acs_id,
                                           int acs_type) {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::onTrackRemoved:"
                << " idx=" << tlist_idx
                << " acs_id=" << acs_id
                << " acs_type=" << acs_type
    ;

  if (components_.size() <= tlist_idx)
    return;
  if (components_[tlist_idx].IsEmpty())
    return;

  DCHECK(owner_);
  v8::Isolate* isolate = isolate_;
  DCHECK(owner_->isolate_r() == isolate);
  v8::HandleScope handle_scope(isolate);

  OipfAvVideoComponent* vc = OipfAvVideoComponent::FromV8Object(
      isolate,
      v8::Local<v8::Object>::New(isolate, components_[tlist_idx]));
  if (vc) {
    DCHECK(acs_type == plugins::OipfVideoPlugin::kAcsTrackTypeVideo);
    components_[tlist_idx].Reset();
    vc->shutdown();
    return;
  }

  OipfAvAudioComponent* ac = OipfAvAudioComponent::FromV8Object(
      isolate,
      v8::Local<v8::Object>::New(isolate, components_[tlist_idx]));
  if (ac) {
    DCHECK(acs_type == plugins::OipfVideoPlugin::kAcsTrackTypeAudio);
    components_[tlist_idx].Reset();
    ac->shutdown();
    return;
  }

  DCHECK(0);
  components_[tlist_idx].Reset();
}

void OipfVideoPluginObject::onTrackChanged(int acs_type) {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPluginObject::onTrackChanged:"
                << " acs_type=" << acs_type
    ;

  WebViewPlugin* wvp = owner_->plugin();
  if (!wvp)
    return;

  int oipf_type;
  if (acs_type == plugins::OipfVideoPlugin::kAcsTrackTypeAudio)
    oipf_type = OipfAvComponent::kTypeAudio;
  else if (acs_type == plugins::OipfVideoPlugin::kAcsTrackTypeVideo)
    oipf_type = OipfAvComponent::kTypeVideo;
  else {
    // FIXME: impl. subtitle.
    return;
  }

  // outside plugin
  blink::WebPluginContainer* container = wvp->container();
  if (!container)
    return;

  blink::WebLocalFrame* r_frame_0 = 0;
  blink::WebElement r_element = container->element();
  blink::WebDocument r_document = r_element.document();
  r_frame_0 = r_document.frame();
  blink::WebLocalFrame* r_frame_1 = owner_->render_frame()->GetWebFrame();
  DCHECK(r_frame_0 == r_frame_1);
  (void)r_frame_0;
  (void)r_frame_1;

  blink::WebLocalFrame* r_frame = r_frame_0;
  if (!r_frame)
    return;

  v8::Isolate* isolate = isolate_;
  DCHECK(owner_->isolate_r() == isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(r_frame->mainWorldScriptContext());

  v8::Handle<v8::Object> r_element_obj = container->v8ObjectForElement();
  if (r_element_obj->IsNull() || !r_element_obj->IsObject())
    return;

  v8::MaybeLocal<v8::String> mbl;
  v8::Local<v8::String> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  // DOM 2 Event
  // use break to exit.
  do {
    v8::Local<v8::Object> r_window_obj =
        r_frame->mainWorldScriptContext()->Global();
    mbl = v8::String::NewFromUtf8(isolate, "document",
                                  v8::String::kNormalString, 8);
    if (!mbl.ToLocal(&key))
      break;
    prop = r_window_obj->Get(key);
    if (prop->IsNull() || !prop->IsObject())
      break;
    v8::Handle<v8::Object> r_document_obj = v8::Handle<v8::Object>::Cast(prop);

    mbl = v8::String::NewFromUtf8(isolate, "createEvent",
                                  v8::String::kNormalString, 11);
    if (!mbl.ToLocal(&key))
      break;
    prop = r_document_obj->Get(key);
    if (prop->IsNull() || !prop->IsFunction())
      break;
    function = v8::Handle<v8::Function>::Cast(prop);
    v8::Local<v8::Object> event_obj;
    {
      v8::MaybeLocal<v8::String> p0_mbl = v8::String::NewFromUtf8(
          isolate, "Event", v8::String::kNormalString, 5);
      v8::Local<v8::Value> p0;
      if (!p0_mbl.ToLocal(&p0))
        break;
      v8::Handle<v8::Value> argv[] = { p0 };
      v8::TryCatch try_catch(isolate);
      v8::Handle<v8::Value> result = r_frame->callFunctionEvenIfScriptDisabled(
          function, r_document_obj, 1, argv);
      if (try_catch.HasCaught())
        break;
      if (result->IsNull() || !result->IsObject())
        break;
      event_obj = v8::Handle<v8::Function>::Cast(result);
    }

    mbl = v8::String::NewFromUtf8(isolate, "initEvent",
                                  v8::String::kNormalString, 9);
    if (!mbl.ToLocal(&key))
      break;
    prop = event_obj->Get(key);
    if (prop->IsNull() || !prop->IsFunction())
      break;
    function = v8::Handle<v8::Function>::Cast(prop);
    {
      v8::MaybeLocal<v8::String> p0_mbl = v8::String::NewFromUtf8(
          isolate, "SelectedComponentChange", v8::String::kNormalString, 23);
      v8::Local<v8::Value> p0;
      if (!p0_mbl.ToLocal(&p0))
        break;
      v8::Handle<v8::Value> p1 = v8::Boolean::New(isolate, false);
      v8::Handle<v8::Value> p2 = v8::Boolean::New(isolate, false);
      v8::Handle<v8::Value> argv[] = { p0, p1, p2 };
      v8::TryCatch try_catch(isolate);
      v8::Handle<v8::Value> result = r_frame->callFunctionEvenIfScriptDisabled(
          function, event_obj, 3, argv);
      if (try_catch.HasCaught())
        break;
      (void)result;
    }

    mbl = v8::String::NewFromUtf8(isolate, "componentType",
                                  v8::String::kNormalString, 13);
    if (!mbl.ToLocal(&key))
      break;
    prop = v8::Number::New(isolate, oipf_type);
    event_obj->Set(key, prop);

    mbl = v8::String::NewFromUtf8(isolate, "dispatchEvent",
                                  v8::String::kNormalString, 13);
    if (!mbl.ToLocal(&key))
      break;
    prop = r_element_obj->Get(key);
    if (prop->IsNull() || !prop->IsFunction())
      break;
    function = v8::Handle<v8::Function>::Cast(prop);
    {
      v8::Handle<v8::Value> argv[] = { event_obj };
      v8::TryCatch try_catch(isolate);
      v8::Handle<v8::Value> result = r_frame->callFunctionEvenIfScriptDisabled(
          function, r_element_obj, 1, argv);
      (void)result;
    }
  } while (0);

  // onSelectedComponentChange
  mbl = v8::String::NewFromUtf8(isolate, "onSelectedComponentChange",
                                v8::String::kNormalString, 25);
  if (!mbl.ToLocal(&key))
    return;
  prop = r_element_obj->Get(key);
  if (prop->IsNull() || !prop->IsFunction())
    return;
  function = v8::Handle<v8::Function>::Cast(prop);
  {
    v8::Handle<v8::Value> p0 = v8::Number::New(isolate, oipf_type);
    v8::Handle<v8::Value> argv[] = { p0 };
    v8::TryCatch try_catch(isolate);
    v8::Handle<v8::Value> result = r_frame->callFunctionEvenIfScriptDisabled(
        function, r_element_obj, 1, argv);
    (void)result;
  }
}

double OipfVideoPluginObject::GetAspectRatio(
    OipfAvVideoComponent* self, int index, int id) {
  if (!self)
    return 0.0/0.0;
  if (!owner_)
    return 0.0/0.0;

  // check if |self| is still valid.
  if (components_.size() <= index)
    return 0.0/0.0;
  if (components_[index].IsEmpty())
    return 0.0/0.0;
  {
    WebViewPlugin* wvp = owner_->plugin();
    if (!wvp)
      return 0.0/0.0;
    // outside plugin
    blink::WebPluginContainer* container = wvp->container();
    blink::WebLocalFrame* r_frame_0 = 0;
    if (!container)
      return 0.0/0.0;
    blink::WebElement r_element = container->element();
    blink::WebDocument r_document = r_element.document();
    r_frame_0 = r_document.frame();
    blink::WebLocalFrame* r_frame_1 = owner_->render_frame()->GetWebFrame();
    DCHECK(r_frame_0 == r_frame_1);
    (void)r_frame_0;
    (void)r_frame_1;
    blink::WebLocalFrame* r_frame = r_frame_0;
    if (!r_frame)
      return 0.0/0.0;
    v8::Isolate* isolate = isolate_;
    DCHECK(owner_->isolate_r() == isolate);
    v8::HandleScope handle_scope(isolate);
    OipfAvVideoComponent* track = OipfAvVideoComponent::FromV8Object(
        isolate, v8::Local<v8::Object>::New(isolate, components_[index]));
    if (track != self)
      return 0.0/0.0;
  }

  double r = owner_->GetAspectRatio(this, index, id);

  return r;
}

}  // namespace content
