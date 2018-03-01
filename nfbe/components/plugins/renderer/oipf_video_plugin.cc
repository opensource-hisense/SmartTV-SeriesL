//Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#if defined(HBBTV_ON)

#include "components/plugins/renderer/oipf_video_plugin.h"

#include <math.h>
#include <stdint.h>

// FIXME: scriptabilty should be moved to other place.

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "cc/blink/web_layer_impl.h"
#include "cc/layers/layer.h"
#include "content/common/frame_messages.h"
#include "content/public/common/content_constants.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/oipf_plugin/oipf_avcomponentcollection.h"
#include "content/renderer/oipf_plugin/oipf_video_plugin_object.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "gin/public/gin_embedders.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerClient.h"
#include "third_party/WebKit/public/web/WebConsoleMessage.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "ui/base/webui/jstemplate_builder.h"

using blink::WebFrame;
using blink::WebPlugin;
using blink::WebURLRequest;

#define X_LOG_V 0

namespace {

const char kHandlePSC[] = "handlePSC";
const char kInitialPlaybackRate[] = "initialPlaybackRate";
const char kLoadUrl[] = "loadUrl";
const char kOnTrackAdded[] = "onTrackAdded";
const char kOnTrackChanged[] = "onTrackChanged";
const char kOnTrackRemoved[] = "onTrackRemoved";
const char kOnVideoSizeChanged[] = "onVideoSizeChanged";
const char kVideoW[] = "videoW";
const char kVideoH[] = "videoH";
const char kVisibleW[] = "visibleW";
const char kVisibleH[] = "visibleH";

// the values here must match those in oipf_video_plugin.html.
const int kEventTypeStopped = 1;
const int kEventTypePlay = 2;
const int kEventTypePaused = 3;
const int kEventTypeEnded = 4;
const int kEventTypeSeeking = 5;
const int kEventTypeSeeked = 6;
const int kEventTypeError = 7;
const int kEventTypeRatechange = 8;

// ./ui/webui/resources/js/load_time_data.js
// "loadTimeData.data =" the dictionary to be passed
// ./ui/webui/resources/js/i18n_template_no_process.js
// "i18nTemplate.process(document, loadTimeData);"

std::string HtmlData(const blink::WebPluginParams& params,
                     base::StringPiece template_html,
                     plugins::OipfVideoPlugin::PluginConfig& config) {

  config.enable_composite_kludge = 1;

  bool f_mime_type_is_x_oipf_video = true;
  bool f_is_audio = false;
  bool f_is_video = false;
  bool f_maybe_audio = false;
  bool f_maybe_video = false;
  std::string orig_mime_type = params.mimeType.utf8();
  if (orig_mime_type == "video/mp4") {
    f_mime_type_is_x_oipf_video = false;
    f_maybe_video = true;
  } else if (orig_mime_type == "audio/mp4" ||
             orig_mime_type == "audio/mpeg") {
    f_mime_type_is_x_oipf_video = false;
    f_maybe_audio = true;
  }
  base::DictionaryValue values;
  values.SetString("key_00", "the_value_00");
  for (size_t i = 0; i < params.attributeNames.size(); ++i) {
    if (params.attributeNames[i] == "videourl" &&
        f_mime_type_is_x_oipf_video) {
      values.SetString("mediaurl", params.attributeValues[i]);
      f_is_video = true;
      continue;
    }
    if (params.attributeNames[i] == "audiourl" &&
        f_mime_type_is_x_oipf_video) {
      values.SetString("mediaurl", params.attributeValues[i]);
      f_is_audio = true;
      continue;
    }
    if (params.attributeNames[i] == "data" && !f_mime_type_is_x_oipf_video) {
      values.SetString("mediaurl", params.attributeValues[i]);
      f_is_audio = f_maybe_audio;
      f_is_video = f_maybe_video;
    }
    if (params.attributeNames[i] == "ignore_layout") {
      values.SetString("ignore_layout", params.attributeValues[i]);
      continue;
    }
    if (params.attributeNames[i] == "enable_composite_kludge") {
      // the last one is effective.
      if (params.attributeValues[i] == "0")
        config.enable_composite_kludge = 0;
      continue;
    }
    if (params.attributeNames[i] == "loop") {
      values.SetString("loop", params.attributeValues[i]);
    }
  }
  if (f_is_video)
    values.SetString("is_video", "true");
  else if (f_is_audio)
    values.SetString("is_video", "false");
  values.SetString("enable_composite_kludge",
                   std::to_string(config.enable_composite_kludge));
  // values.SetString("ignore_layout", "t");  // for debug UI.
  std::string html_data = webui::GetI18nTemplateHtml(template_html, &values);
  if (false)
  {
    VLOG(0) << "ZZZZ: HtmlData: " << html_data;
  }
  return html_data;
}

}  // namespace

namespace plugins {

gin::WrapperInfo OipfVideoPlugin::kWrapperInfo = {gin::kEmbedderNativeGin};

// ? ? ? is this v8::Isolate::GetCurrent() the right isolate
// for plugin side scriptable object ? ? ?

OipfVideoPlugin::OipfVideoPlugin(content::RenderFrame* render_frame,
                                 blink::WebLocalFrame* frame,
                                 const blink::WebPluginParams& params,
                                 base::StringPiece template_html)
    : PluginPlaceholderBase(render_frame,
                            frame,
                            params,
                            HtmlData(params, template_html, config_)),
      gin::NamedPropertyInterceptor(v8::Isolate::GetCurrent(), this),  // FIXME
      visible_w_(-1),
      visible_h_(-1),
      video_w_(-1),
      video_h_(-1),
      initial_playback_rate_(-1),
      media_web_layer_(nullptr),
      isolate_p_(v8::Isolate::GetCurrent()),  // FIXME
      isolate_r_(v8::Isolate::GetCurrent()),
      weak_ptr_factory_(this) {
}

OipfVideoPlugin::~OipfVideoPlugin() {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::dtor:";
  DCHECK(object_r_.IsEmpty());
}

v8::Local<v8::Value> OipfVideoPlugin::GetV8Handle(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, this).ToV8();
}

gin::ObjectTemplateBuilder OipfVideoPlugin::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<OipfVideoPlugin>::GetObjectTemplateBuilder(isolate)
      .AddNamedPropertyInterceptor();
}

v8::Local<v8::Value> OipfVideoPlugin::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  if (identifier == kHandlePSC) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPlugin::handlePSC,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kInitialPlaybackRate) {
    return v8::Integer::New(isolate, initial_playback_rate_);
  }
  if (identifier == kLoadUrl) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPlugin::loadUrl,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kOnTrackAdded) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPlugin::onTrackAdded,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kOnTrackChanged) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPlugin::onTrackChanged,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kOnTrackRemoved) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPlugin::onTrackRemoved,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kOnVideoSizeChanged) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPlugin::onVideoSizeChanged,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  if (identifier == kOnVideoSizeChanged) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&OipfVideoPlugin::onVideoSizeChanged,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kVideoW) {
    return v8::Integer::New(isolate, video_w_);
  }
  if (identifier == kVideoH) {
    return v8::Integer::New(isolate, video_h_);
  }
  if (identifier == kVisibleW) {
    return v8::Integer::New(isolate, visible_w_);
  }
  if (identifier == kVisibleH) {
    return v8::Integer::New(isolate, visible_h_);
  }
  return v8::Local<v8::Value>();
}

bool OipfVideoPlugin::SetNamedProperty(v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (identifier == kHandlePSC) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name handlePSC")));
    return true;
  }
  if (identifier == kInitialPlaybackRate) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name initialPlaybackRate")));
    return true;
  }
  if (identifier == kLoadUrl) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name loadUrl")));
    return true;
  }
  if (identifier == kOnTrackAdded) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name onTrackAdded")));
    return true;
  }
  if (identifier == kOnTrackChanged) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name onTrackChanged")));
    return true;
  }
  if (identifier == kOnTrackRemoved) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name onTrackRemoved")));
    return true;
  }
  if (identifier == kVisibleW) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name visibleW")));
    return true;
  }
  if (identifier == kVisibleH) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name visibleH")));
    return true;
  }
  if (identifier == kVideoW) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name videoW")));
    return true;
  }
  if (identifier == kVideoH) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Cannot set properties with the name videoH")));
    return true;
  }
  return false;
}

std::vector<std::string> OipfVideoPlugin::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kHandlePSC);
  result.push_back(kInitialPlaybackRate);
  result.push_back(kLoadUrl);
  result.push_back(kOnTrackAdded);
  result.push_back(kOnTrackChanged);
  result.push_back(kOnTrackRemoved);
  result.push_back(kVisibleW);
  result.push_back(kVisibleH);
  result.push_back(kVideoW);
  result.push_back(kVideoH);
  return result;
}

void OipfVideoPlugin::setInitialPlayState(double playback_rate)
{
  initial_playback_rate_ = playback_rate;
}

void OipfVideoPlugin::loadUrl(gin::Arguments* args) {
  std::string url_string("http://tcs_dugong:8000/bm");
  GURL url(url_string);
  WebURLRequest request;
  request.initialize();
  request.setURL(url);
  render_frame()->LoadURLExternally(request, blink::WebNavigationPolicyNewForegroundTab);
  args->Return(v8::Local<v8::Value>());
}

void OipfVideoPlugin::onVideoSizeChanged(gin::Arguments* args) {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::onVideoSizeChanged:"
    ;

  // inside plugin
  DCHECK(isolate_p_ == args->isolate());
  v8::Isolate* isolate = args->isolate();

  v8::Handle<v8::Value> v_tmp;

  if (!args->GetNext(&v_tmp)) {
    args->ThrowError();
    return;
  }
  if (!v_tmp->IsNumber() && !v_tmp->IsNumberObject()) {
    args->ThrowError();
    return;
  }
  double vw_d = v_tmp->ToNumber(isolate)->Value();

  if (!args->GetNext(&v_tmp)) {
    args->ThrowError();
    return;
  }
  if (!v_tmp->IsNumber() && !v_tmp->IsNumberObject()) {
    args->ThrowError();
    return;
  }
  double vh_d = v_tmp->ToNumber(isolate)->Value();

  video_w_ = vw_d;
  video_h_ = vh_d;

  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::onVideoSizeChanged:"
                << " " << video_w_ << "x" << video_h_
    ;

  args->Return(v8::Local<v8::Value>());
}

void OipfVideoPlugin::PluginDestroyed() {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::PluginDestroyed:"
    ;
  if (!object_r_.IsEmpty()) {
    v8::HandleScope handle_scope(isolate_r_);
    v8::Local<v8::Object> lh = v8::Local<v8::Object>::New(isolate_r_,
                                                          object_r_);
    content::OipfVideoPluginObject* plugin_object =
        content::OipfVideoPluginObject::FromV8Object(isolate_r_, lh);
    DCHECK(plugin_object);
    plugin_object->shutdown();
    object_r_.Reset();
  }
  PluginPlaceholderBase::PluginDestroyed();
}

v8::Local<v8::Object> OipfVideoPlugin::GetV8ScriptableObject_1(
    v8::Isolate* isolate) {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::GetV8ScriptableObject_1:"
    ;

  DCHECK_EQ(isolate, isolate_r_);

  // we cannot call scriptable here, because it is that scriptable
  // which needs the wrapper object of this.
  // e.g. calling container_element->Get() results again in calling
  // this GetV8ScriptableObject_1() because there is yet no wrapper
  // object for this, i.e. infinite recursion.
  if (!init_v8_object_timer_.IsRunning()) {
    init_v8_object_timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(1),
        base::Bind(&OipfVideoPlugin::addFullscreenStretchClass,
                   weak_ptr_factory_.GetWeakPtr()));
  }

  content::OipfVideoPluginObject* plugin_object =
      new content::OipfVideoPluginObject(this, isolate);
  object_r_.Reset(isolate, plugin_object->GetWrapper(isolate));

  return v8::Local<v8::Object>::New(isolate, object_r_);
}

void OipfVideoPlugin::addFullscreenStretchClass() {
  DCHECK(content::RenderThread::Get());
  WebViewPlugin* wvp = plugin();
  if (!wvp)
    return;
  // outside plugin
  blink::WebPluginContainer* container = wvp->container();
  if (!container)
    return;
  blink::WebElement r_element = container->element();
  blink::WebDocument r_document = r_element.document();
  blink::WebLocalFrame* r_frame_0 = r_document.frame();
  blink::WebLocalFrame* r_frame = r_frame_0;
  if (!r_frame)
    return;
  v8::HandleScope handle_scope(isolate_r_);
  v8::Context::Scope context_scope(r_frame->mainWorldScriptContext());
  v8::Handle<v8::Object> r_element_obj = container->v8ObjectForElement();
  if (r_element_obj->IsNull() || !r_element_obj->IsObject())
    return;
  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;
  key = v8::String::NewFromUtf8(isolate_r_, "classList",
      v8::String::kNormalString, 9);
  prop = r_element_obj->Get(key);
  if (prop->IsNull() || !prop->IsObject())
    return;
  v8::Handle<v8::Object> token_list_obj = v8::Handle<v8::Object>::Cast(prop);
  key = v8::String::NewFromUtf8(isolate_r_, "add", v8::String::kNormalString, 3);
  prop = token_list_obj->Get(key);
  if (prop->IsNull() || !prop->IsFunction())
    return;
  function = v8::Handle<v8::Function>::Cast(prop);
  v8::Handle<v8::Value> p0 = v8::String::NewFromUtf8(isolate_r_,
      "acs_object_video", v8::String::kNormalString, 16);
  v8::Handle<v8::Value> argv[] = { p0 };
  v8::Handle<v8::Value> result = r_frame->callFunctionEvenIfScriptDisabled(
      function, token_list_obj, 1, argv);
  (void)result;
}

void OipfVideoPlugin::OnResize(const blink::WebSize size) {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::OnResize:"
                << " width=" << size.width
                << " height=" << size.height;

  if (config_.enable_composite_kludge) {
    // make the WebView itself transparent to see (the background of)
    // the container <object> element.
    // n.b. the content of the WebView must also be transparent.
    // see the content document for how to make it transparent.
    WebViewPlugin* wvp = plugin();
    if (wvp) {
      wvp->setIsTransparent(true);
    }
  }

  visible_w_ = size.width;
  visible_h_ = size.height;

  // for debug, you can add watchpoints on the cc::Layer of |media_web_layer_|,
  // whose parameters are modified during layout.

  // for some unknown reason, the current context does not allow JS.
  // calling JS function here does nothing, and _the next_ JS execution
  // where JS is permitted results in "Script execution is forbidden."
  // error.

  if (true) {
    // do not call JS here....
    if (!onresize_timer_.IsRunning()) {
      onresize_timer_.Start(
          FROM_HERE,
          base::TimeDelta::FromMilliseconds(100),
          base::Bind(&OipfVideoPlugin::OnResizeDelayed,
                     weak_ptr_factory_.GetWeakPtr()));
    }
    return;
  }

  OnResizeInternal();
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::OnResize: EXIT";
}

void OipfVideoPlugin::OnResizeDelayed() {
  DCHECK(content::RenderThread::Get());
  OnResizeInternal();
}

void OipfVideoPlugin::OnResizeInternal() {
  // inside plugin
  WebViewPlugin* wvp = plugin();
  if (!wvp) {
    return;
  }
  blink::WebView* wv = wvp->web_view();
  blink::WebFrame* p_frame = wv->mainFrame();

  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::OnResizeInternal:"
                << " p_frame=" << static_cast<void*>(p_frame)
                << " r_frame_1=" << render_frame()->GetWebFrame()
    ;

  v8::HandleScope handle_scope(isolate_p_);
  v8::Context::Scope context_scope(p_frame->mainWorldScriptContext());

  v8::Local<v8::Object> p_obj = GetWrapper(isolate_p_);

  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  key = v8::String::NewFromUtf8(isolate_p_, "onresize", v8::String::kNormalString, 8);
  prop = p_obj->Get(key);
  if (!prop->IsFunction()) {
    // this happens when the plugin is not yet fully initialized.
    // VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::OnResizeDelayed: onresize is not a function";
    return;
  }
  function = v8::Handle<v8::Function>::Cast(prop);

  {
    // 'w' : integer
    v8::Handle<v8::Value> p0 = v8::Integer::New(isolate_p_, visible_w_);
    // 'h' : integer
    v8::Handle<v8::Value> p1 = v8::Integer::New(isolate_p_, visible_h_);
    v8::Handle<v8::Value> argv[] = { p0, p1 };
    v8::Handle<v8::Value> result = p_frame->callFunctionEvenIfScriptDisabled(
        function, p_obj, 2, argv);
    (void)result;
  }

#if 0
  // check if the video is really resized.
  if (config_.enable_composite_kludge) {
    if (media_web_layer_) {
      cc::Layer* media_layer = static_cast<cc_blink::WebLayerImpl*>(media_web_layer_)
          ->layer();
      int vw = media_layer->bounds().width();
      int vh = media_layer->bounds().height();
      VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::OnResizeInternal:"
                    << " vw=" << vw
                    << " vh=" << vh
        ;
    }
  }
#endif

  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::OnResizeInternal: EXIT";

}

void OipfVideoPlugin::handlePSC(gin::Arguments* args) {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::handlePSC:"
    ;

  // inside plugin
  WebViewPlugin* wvp = plugin();
  if (!wvp) {
    return;
  }
  blink::WebView* wv = wvp->web_view();
  blink::WebFrame* p_frame = wv->mainFrame();

  DCHECK(isolate_p_ == args->isolate());

  v8::Handle<v8::Value> v_tmp;
  v8::Handle<v8::Array> v_oQ;

  if (!args->GetNext(&v_tmp)) {
    return;
  }
  if (!v_tmp->IsArray()) {
    return;
  }

  v_oQ = v8::Handle<v8::Array>::Cast(v_tmp);

  uint32_t v_oQ_length = v_oQ->Length();

  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::handlePSC: length=" << v_oQ_length;

  v8::Handle<v8::Value> v_key_type = v8::String::NewFromUtf8(
      isolate_p_, "type", v8::String::kNormalString, 4);
  v8::Handle<v8::Value> v_key_param = v8::String::NewFromUtf8(
      isolate_p_, "param", v8::String::kNormalString, 5);

  for (uint32_t i = 0; i < v_oQ_length; i += 1) {
    v_tmp = v_oQ->Get(v8::Integer::New(isolate_p_, i));
    if (v_tmp->IsNull() || !v_tmp->IsObject())
      continue;
    v8::Local<v8::Object> v_elem = v8::Handle<v8::Object>::Cast(v_tmp);
    v_tmp = v_elem->Get(v_key_type);
    if (v_tmp->IsNull() || !v_tmp->IsNumber())
      continue;
    int ev_type = (int)(v_tmp->ToNumber(isolate_p_)->Value());
    {
      const char* ev_type_name = "__unknown__";
      switch (ev_type) {
      case kEventTypeStopped: ev_type_name = "stopped"; break;
      case kEventTypePlay: ev_type_name = "play"; break;
      case kEventTypePaused: ev_type_name = "paused"; break;
      case kEventTypeEnded: ev_type_name = "ended"; break;
      case kEventTypeSeeking: ev_type_name = "seeking"; break;
      case kEventTypeSeeked: ev_type_name = "seeked"; break;
      case kEventTypeError: ev_type_name = "error"; break;
      case kEventTypeRatechange: ev_type_name = "ratechange"; break;
      }
      VLOG(X_LOG_V) << " at " << i << " " << ev_type_name;
    }
    double ev_param = 0;
    int oipf_event_type = 0;  // 'PlayStateChange'
    switch (ev_type) {
    case kEventTypeStopped:
      break;
    case kEventTypePlay:
      break;
    case kEventTypePaused:
      break;
    case kEventTypeEnded:
      break;
    case kEventTypeSeeking:
      continue;
    case kEventTypeSeeked:
      oipf_event_type = 2;  // 'PlayPositionChanged'
      v_tmp = v_elem->Get(v_key_param);
      if (v_tmp->IsNull() || !v_tmp->IsNumber())
        continue;
      ev_param = v_tmp->ToNumber(isolate_p_)->Value();
      break;
    case kEventTypeError:
      break;
    case kEventTypeRatechange:
      oipf_event_type = 1;  // 'PlaySpeedChanged'
      v_tmp = v_elem->Get(v_key_param);
      if (v_tmp->IsNull() || !v_tmp->IsNumber())
        continue;
      ev_param = v_tmp->ToNumber(isolate_p_)->Value();
      break;
    default:
      continue;
    }
    {
      // outside plugin
      blink::WebPluginContainer* container = wvp->container();
      if (!container)
        break;
      blink::WebElement r_element = container->element();
      blink::WebDocument r_document = r_element.document();
      blink::WebLocalFrame* r_frame_0 = r_document.frame();
      blink::WebLocalFrame* r_frame = r_frame_0;
      if (!r_frame)
        break;
      v8::HandleScope handle_scope(isolate_r_);
      v8::Context::Scope context_scope(r_frame->mainWorldScriptContext());
      v8::Handle<v8::Object> r_element_obj = container->v8ObjectForElement();
      if (r_element_obj->IsNull() || !r_element_obj->IsObject())
        break;
      v8::Handle<v8::Value> key;
      v8::Handle<v8::Value> prop;
      v8::Handle<v8::Function> function;
      // DOM 2 Event
      v8::Local<v8::Object> r_window_obj =
          r_frame->mainWorldScriptContext()->Global();
      key = v8::String::NewFromUtf8(isolate_r_, "document",
                                    v8::String::kNormalString, 8);
      prop = r_window_obj->Get(key);
      if (prop->IsNull() || !prop->IsObject())
        break;
      v8::Handle<v8::Object> r_document_obj = v8::Handle<v8::Object>::Cast(prop);
      key = v8::String::NewFromUtf8(isolate_r_, "createEvent",
                                    v8::String::kNormalString, 11);
      prop = r_document_obj->Get(key);
      if (prop->IsNull() || !prop->IsFunction())
        break;
      function = v8::Handle<v8::Function>::Cast(prop);
      v8::Local<v8::Object> event_obj;
      {
        v8::Handle<v8::Value> p0 = v8::String::NewFromUtf8(isolate_r_, "Event",
            v8::String::kNormalString, 5);
        v8::Handle<v8::Value> argv[] = { p0 };
        v8::Handle<v8::Value> result = r_frame->callFunctionEvenIfScriptDisabled(
            function, r_document_obj, 1, argv);
        if (prop->IsNull() || !result->IsObject())
          break;
        event_obj = v8::Handle<v8::Function>::Cast(result);
      }
      key = v8::String::NewFromUtf8(isolate_r_, "initEvent",
                                    v8::String::kNormalString, 9);
      prop = event_obj->Get(key);
      if (prop->IsNull() || !prop->IsFunction())
        break;
      function = v8::Handle<v8::Function>::Cast(prop);
      {
        v8::Handle<v8::Value> p0;
        if (1 == oipf_event_type) {
          p0 = v8::String::NewFromUtf8(isolate_r_, "PlaySpeedChanged",
              v8::String::kNormalString, 16);
        } else if (2 == oipf_event_type) {
          p0 = v8::String::NewFromUtf8(isolate_r_, "PlayPositionChanged",
              v8::String::kNormalString, 19);
        } else {
          p0 = v8::String::NewFromUtf8(isolate_r_, "PlayStateChange",
              v8::String::kNormalString, 15);
        }
        v8::Handle<v8::Value> p1 = v8::Boolean::New(isolate_r_, false);
        v8::Handle<v8::Value> p2 = v8::Boolean::New(isolate_r_, false);
        v8::Handle<v8::Value> argv[] = { p0, p1, p2 };
        v8::Handle<v8::Value> result = r_frame->callFunctionEvenIfScriptDisabled(
            function, event_obj, 3, argv);
        // FIXME: check thrown error.
        (void)result;
      }
      if (1 == oipf_event_type) {
        key = v8::String::NewFromUtf8(isolate_r_, "speed",
                                      v8::String::kNormalString, 5);
        prop = v8::Number::New(isolate_r_, ev_param);
        event_obj->Set(key, prop);
      } else if (2 == oipf_event_type) {
        key = v8::String::NewFromUtf8(isolate_r_, "position",
                                      v8::String::kNormalString, 8);
        prop = v8::Number::New(isolate_r_, ev_param);
        event_obj->Set(key, prop);
      }
      //
      key = v8::String::NewFromUtf8(isolate_r_, "dispatchEvent",
                                    v8::String::kNormalString, 13);
      prop = r_element_obj->Get(key);
      if (prop->IsNull() || !prop->IsFunction())
        break;
      function = v8::Handle<v8::Function>::Cast(prop);
      {
        v8::Handle<v8::Value> argv[] = { event_obj };
        v8::Handle<v8::Value> result = r_frame->callFunctionEvenIfScriptDisabled(
            function, r_element_obj, 1, argv);
        // FIXME: check if an error is thrown.
        (void)result;
      }
      // onXXXX
      if (1 == oipf_event_type) {
        key = v8::String::NewFromUtf8(isolate_r_, "onPlaySpeedChanged",
                                      v8::String::kNormalString, 18);
      } else if (2 == oipf_event_type) {
        key = v8::String::NewFromUtf8(isolate_r_, "onPlayPositionChanged",
                                      v8::String::kNormalString, 21);
      } else {
        key = v8::String::NewFromUtf8(isolate_r_, "onPlayStateChange",
                                      v8::String::kNormalString, 17);
      }
      prop = r_element_obj->Get(key);
      if (prop->IsNull() || !prop->IsFunction())
        break;
      function = v8::Handle<v8::Function>::Cast(prop);
      if (1 == oipf_event_type) {
        v8::Handle<v8::Value> p0 = v8::Number::New(isolate_r_, ev_param);
        v8::Handle<v8::Value> argv[] = { p0 };
        v8::Handle<v8::Value> result = r_frame->callFunctionEvenIfScriptDisabled(
            function, r_element_obj, 1, argv);
        (void)result;
      } else if (2 == oipf_event_type) {
        v8::Handle<v8::Value> p0 = v8::Number::New(isolate_r_, ev_param);
        v8::Handle<v8::Value> argv[] = { p0 };
        v8::Handle<v8::Value> result = r_frame->callFunctionEvenIfScriptDisabled(
            function, r_element_obj, 1, argv);
        (void)result;
      } else {
        v8::Handle<v8::Value> argv[] = { };
        v8::Handle<v8::Value> result = r_frame->callFunctionEvenIfScriptDisabled(
            function, r_element_obj, 0, argv);
        (void)result;
      }
    }
  }

  (void)p_frame;
}

void OipfVideoPlugin::onTrackAdded(gin::Arguments* args) {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::onTrackAdded:"
    ;

  // inside plugin
  WebViewPlugin* wvp = plugin();
  if (!wvp) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  DCHECK(isolate_p_ == args->isolate());

  v8::Handle<v8::Value> v_tmp;
  double p0_d;  int v_tlist_idx;
  double p1_d;  int v_acs_id;
  double p2_d;  int v_acs_type;

  if (!args->GetNext(&v_tmp)) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  if (!v_tmp->IsNumber() && !v_tmp->IsNumberObject()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  p0_d = v_tmp->ToNumber(isolate_p_)->Value();
  v_tlist_idx = (int)p0_d;

  if (!args->GetNext(&v_tmp)) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  if (!v_tmp->IsNumber() && !v_tmp->IsNumberObject()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  p1_d = v_tmp->ToNumber(isolate_p_)->Value();
  v_acs_id = (int)p1_d;

  if (!args->GetNext(&v_tmp)) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  if (!v_tmp->IsNumber() && !v_tmp->IsNumberObject()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  p2_d = v_tmp->ToNumber(isolate_p_)->Value();
  v_acs_type = (int)p2_d;

  // FIXME: check if |tlist|[v_tlist_idx] exists, and it is really
  // a track object whose acs_id == v_acs_id and acs_type == v_acs_type.

  if (object_r_.IsEmpty()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  v8::Local<v8::Object> lh = v8::Local<v8::Object>::New(
      isolate_r_, object_r_);
  content::OipfVideoPluginObject* plugin_object =
      content::OipfVideoPluginObject::FromV8Object(isolate_r_, lh);
  DCHECK(plugin_object);

  plugin_object->onTrackAdded(v_tlist_idx, v_acs_id, v_acs_type);

  args->Return(v8::Local<v8::Value>());
}

void OipfVideoPlugin::onTrackRemoved(gin::Arguments* args) {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::onTrackRemoved:"
    ;

  // inside plugin
  WebViewPlugin* wvp = plugin();
  if (!wvp) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  DCHECK(isolate_p_ == args->isolate());

  v8::Handle<v8::Value> v_tmp;
  double p0_d;  int v_tlist_idx;
  double p1_d;  int v_acs_id;
  double p2_d;  int v_acs_type;

  if (!args->GetNext(&v_tmp)) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  if (!v_tmp->IsNumber() && !v_tmp->IsNumberObject()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  p0_d = v_tmp->ToNumber(isolate_p_)->Value();
  v_tlist_idx = (int)p0_d;

  if (!args->GetNext(&v_tmp)) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  if (!v_tmp->IsNumber() && !v_tmp->IsNumberObject()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  p1_d = v_tmp->ToNumber(isolate_p_)->Value();
  v_acs_id = (int)p1_d;

  if (!args->GetNext(&v_tmp)) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  if (!v_tmp->IsNumber() && !v_tmp->IsNumberObject()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  p2_d = v_tmp->ToNumber(isolate_p_)->Value();
  v_acs_type = (int)p2_d;

  // FIXME: check if |tlist|[v_tlist_idx] is really null.

  if (object_r_.IsEmpty()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  v8::Local<v8::Object> lh = v8::Local<v8::Object>::New(isolate_r_,
                                                        object_r_);
  content::OipfVideoPluginObject* plugin_object =
      content::OipfVideoPluginObject::FromV8Object(isolate_r_, lh);
  DCHECK(plugin_object);

  plugin_object->onTrackRemoved(v_tlist_idx, v_acs_id, v_acs_type);

  args->Return(v8::Local<v8::Value>());
}

void OipfVideoPlugin::onTrackChanged(gin::Arguments* args) {
  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::onTrackChanged:"
    ;

  // inside plugin
  WebViewPlugin* wvp = plugin();
  if (!wvp) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  DCHECK(isolate_p_ == args->isolate());

  v8::Handle<v8::Value> v_tmp;
  double p0_d;  int v_acs_type;

  if (!args->GetNext(&v_tmp)) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  if (!v_tmp->IsNumber() && !v_tmp->IsNumberObject()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  p0_d = v_tmp->ToNumber(isolate_p_)->Value();
  v_acs_type = (int)p0_d;

  if (object_r_.IsEmpty()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  v8::Local<v8::Object> lh = v8::Local<v8::Object>::New(
      isolate_r_, object_r_);
  content::OipfVideoPluginObject* plugin_object =
      content::OipfVideoPluginObject::FromV8Object(isolate_r_, lh);
  DCHECK(plugin_object);

  plugin_object->onTrackChanged(v_acs_type);

  args->Return(v8::Local<v8::Value>());
}

blink::WebMediaPlayer* OipfVideoPlugin::createMediaPlayer(
    const blink::WebURL& url,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
    blink::WebContentDecryptionModule* initial_cdm,
    const blink::WebString& sinkId,
    blink::WebMediaSession* wm_session) {
  // this is a RenderFrameObserver.
  content::RenderFrame* rf = render_frame();
  if (!rf) {
    // RenderFrameGone() ocurred.
    return 0;
  }

  // |client| is HTMLMediaElement.
  // when the media player creates its video layer,
  // it does |client|->setWebLayer().
  // obtain that web layer, and does container->setWebLayer(that_web_layer);
  client->setAcsSetWebLayerDelegate(this);
  client->setWebLayer(0);

  content::RenderFrameImpl* rfi = static_cast<content::RenderFrameImpl*>(rf);
  return rfi->createMediaPlayer(url, client, encrypted_client,
      initial_cdm, sinkId, wm_session);
}

bool OipfVideoPlugin::hasContentsLayer() const {
  if (media_web_layer_)
    return true;
  return false;
}

blink::WebSize OipfVideoPlugin::contentsLayerIntrinsicSize() const {
  if (false)
  {
    VLOG(X_LOG_V) << "ZZZZ: OipfVideoPlugin::contentsLayerIntrinsicSize:"
                  << " " << video_w_ << "x" << video_h_
      ;
  }
  if (media_web_layer_ && 0 < video_w_ && 0 < video_h_) {
    // FIXME: ideally, we just need to call |our_webMediaPlayer|->naturalSize(),
    // but we do not have an ownership of our webMediaPlayer....
    return blink::WebSize(video_w_, video_h_);
  }
  return blink::WebSize();
}

bool OipfVideoPlugin::shouldReportDetailedMessageForSourceWvD(
    const blink::WebString& source) {
#if defined _DEBUG
  return true;
#else
  return false;
#endif
}

void OipfVideoPlugin::DetailedConsoleMessageAddedWvD(
    const base::string16& message,
    const base::string16& source,
    const base::string16& stack_trace,
    int32_t line_number,
    int32_t severity_level) {

  content::RenderFrame* rf = render_frame();
  if (!rf)
    return;

  content::RenderFrameImpl* rfi = static_cast<content::RenderFrameImpl*>(rf);

  Send(new FrameHostMsg_AddMessageToConsole(rfi->GetRoutingID(),
                                            severity_level,
                                            message,
                                            line_number,
                                            source));
}

bool OipfVideoPlugin::onSetWebLayer(blink::WebLayer* media_web_layer) {
  {
    VLOG(0) << "ZZZZ: OipfVideoPlugin::onSetWebLayer:"
            << " from "
            << (media_web_layer_
                ? std::string("id=").append(std::to_string(
                      media_web_layer_->id()))
                : std::string("null"))
            << " to "
            << (media_web_layer
                ? std::string("id=").append(std::to_string(
                      media_web_layer->id()))
                : std::string("null"))
        ;
  }

  if (!config_.enable_composite_kludge)
    return false;

  // |container| is WebPluginContainerImpl.
  WebViewPlugin* wvp = plugin();
  if (!wvp) {
    // WebPluginContainerImpl::dispose() will do or did
    // GraphicsLayer::unregisterContentsLayer(|m_webLayer|),
    // and we are relying on that.
    // FIXME: we should not rely on that.
    return false;
  }
  blink::WebPluginContainer* container = wvp->container();
  if (!container) {
    // WebPluginContainerImpl::dispose() will do or did
    // GraphicsLayer::unregisterContentsLayer(|m_webLayer|),
    // and we are relying on that.
    // FIXME: we should not rely on that.
    return false;
  }

  media_web_layer_ = media_web_layer;

  container->setWebLayer(media_web_layer_);
  return true;
}

// this function assumes that the caller has installed a proper handlescope.
// FIXME: OipfVideoPlugin should implement |tlist| and |t_id|.
// see oipf_video_plugin.html.
//
v8::Local<v8::Value> OipfVideoPlugin::GetTrackFromTlist(v8::Isolate* isolate,
                                                        int tlist_idx,
                                                        int acs_id)
{
  v8::Local<v8::Object> p_obj = GetWrapper(isolate);

  v8::MaybeLocal<v8::String> mbl;
  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;

  mbl = v8::String::NewFromUtf8(isolate, "tlist", v8::String::kNormalString,
                                5);
  if (!mbl.ToLocal(&key))
    return v8::Local<v8::Value>();
  prop = p_obj->Get(key);
  if (prop->IsNull() || !prop->IsArray()) {
    VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::GetTrackFromTlist:"
                  << " tlist is not an array"
      ;
  }

  return v8::Local<v8::Value>();
}

bool OipfVideoPlugin::CheckIfTrackIsSelected(
    content::OipfVideoPluginObject* self, int index, int id) {
  // assume that the caller is outside the plugin.

  // inside plugin
  WebViewPlugin* wvp = plugin();
  if (!wvp)
    return false;
  blink::WebView* wv = wvp->web_view();
  if (!wv)
    return false;
  blink::WebFrame* p_frame = wv->mainFrame();
  if (!p_frame)
    return false;

  v8::HandleScope handle_scope(isolate_p_);
  v8::Context::Scope context_scope(p_frame->mainWorldScriptContext());

  v8::Local<v8::Object> p_obj = GetWrapper(isolate_p_);

  v8::MaybeLocal<v8::String> mbl;
  v8::Local<v8::String> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  mbl = v8::String::NewFromUtf8(isolate_p_, "checkIfTrackIsSelected",
                                v8::String::kNormalString, 22);
  if (!mbl.ToLocal(&key))
    return false;
  prop = p_obj->Get(key);
  if (!prop->IsFunction())
    return false;
  function = v8::Handle<v8::Function>::Cast(prop);

  bool rval = false;
  {
    v8::Handle<v8::Value> p0 = v8::Integer::New(isolate_p_, index);
    v8::Handle<v8::Value> p1 = v8::Integer::New(isolate_p_, id);
    v8::Handle<v8::Value> argv[] = { p0, p1 };
    v8::TryCatch try_catch(isolate_p_);
    v8::Handle<v8::Value> result = p_frame->callFunctionEvenIfScriptDisabled(
        function, p_obj, 2, argv);
    if (try_catch.HasCaught()) {
#if 0
      v8::Local<v8::String> ss;
      v8::MaybeLocal<v8::String> ms = try_catch.Exception()->ToString(
          isolate_p_);
      if (ms.ToLocal(&ss)) {
        char buf[256];
        int r = ss->WriteUtf8(buf, 255, NULL,
                              (v8::String::NO_NULL_TERMINATION|
                               v8::String::REPLACE_INVALID_UTF8));
        DCHECK(r <= 255);
        buf[r] = '\0';
        VLOG(0) << "OipfVideoPlugin::CheckIfTrackIsSelected: error: " << buf;
      }
#endif
      return false;
    }
    if (!result->IsNull() && result->IsBoolean())
      rval = v8::Handle<v8::Boolean>::Cast(result)->IsTrue();
  }

  return rval;
}

bool OipfVideoPlugin::SelectTrack(content::OipfVideoPluginObject* self,
                                  int index, int id, bool select) {
  // assume that the caller is outside the plugin.

  VLOG(X_LOG_V) << "ZZZZ OipfVideoPlugin::SelectTrack:"
                << " index=" << index
                << " id=" << id
                << " select=" << select
    ;

  // inside plugin
  WebViewPlugin* wvp = plugin();
  if (!wvp)
    return false;
  blink::WebView* wv = wvp->web_view();
  if (!wv)
    return false;
  blink::WebFrame* p_frame = wv->mainFrame();
  if (!p_frame)
    return false;

  v8::HandleScope handle_scope(isolate_p_);
  v8::Context::Scope context_scope(p_frame->mainWorldScriptContext());

  v8::Local<v8::Object> p_obj = GetWrapper(isolate_p_);

  v8::MaybeLocal<v8::String> mbl;
  v8::Local<v8::String> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  mbl = v8::String::NewFromUtf8(isolate_p_, "selectTrack",
                                v8::String::kNormalString, 11);
  if (!mbl.ToLocal(&key))
    return false;
  prop = p_obj->Get(key);
  if (!prop->IsFunction())
    return false;
  function = v8::Handle<v8::Function>::Cast(prop);

  bool rval = false;
  {
    v8::Handle<v8::Value> p0 = v8::Integer::New(isolate_p_, index);
    v8::Handle<v8::Value> p1 = v8::Integer::New(isolate_p_, id);
    v8::Handle<v8::Value> p2 = v8::Boolean::New(isolate_p_, select);
    v8::Handle<v8::Value> argv[] = { p0, p1, p2 };
    v8::TryCatch try_catch(isolate_p_);
    v8::Handle<v8::Value> result = p_frame->callFunctionEvenIfScriptDisabled(
        function, p_obj, 3, argv);
    if (try_catch.HasCaught()) {
#if 0
      v8::Local<v8::String> ss;
      v8::MaybeLocal<v8::String> ms = try_catch.Exception()->ToString(
          isolate_p_);
      if (ms.ToLocal(&ss)) {
        char buf[256];
        int r = ss->WriteUtf8(buf, 255, NULL,
                              (v8::String::NO_NULL_TERMINATION|
                               v8::String::REPLACE_INVALID_UTF8));
        DCHECK(r <= 255);
        buf[r] = '\0';
        VLOG(0) << "OipfVideoPlugin::SelectTrack: error: " << buf;
      }
#endif
      return false;
    }
    if (!result->IsNull() && result->IsBoolean())
      rval = v8::Handle<v8::Boolean>::Cast(result)->IsTrue();
  }

  return rval;
}


// [OIPFDAE] 8.4.2 AVComponent
// aspect ratio for MP4 FF is "Not defined".
// so, what is our mapping?
// for AVC in MP4 FF, there are multiple ways to decide, all of them
// potentially return different values.
// 1. AVC has SPSs, but, for MP4 FF, all have the same SAR.  use it.
// 2. MP4 FF should have Avc Configuration box ("avcC") which as SPS.  use it.
// 3. MP4 FF seems to have DAR somewhere....  use it.
// 4. we cannot parse MP4 FF, so use whatever value the video tag returns.
//
double OipfVideoPlugin::GetAspectRatio(
    content::OipfVideoPluginObject* self, int index, int id) {

  // FIXME: check if |tlist|[index] exists, and it is really
  // a track object whose acs_id == |id| and acs_type == 1(video).

  if (video_w_ < 0 || video_h_ < 0)
    return 0.0/0.0;
  if (video_h_ == 0)
    return 0.0/0.0;  // ? ? ? (w == 0 ? NaN : Inf) ? ? ?

  // 7.16.5.3 The AVVideoComponent class
  // 7.16.5.3.1 Properties
  // "Values SHALL be equal to width divided by height, rounded to a float
  // value with two decimals, e.g. 1.78 to indicate 16:9 and 1.33 to
  // indicate 4:3." which is extrelemy difficult when IEEE-754.
  // n.b. the contract says int never overflows even if it actually overflows.
  uint64_t w = video_w_;
  uint64_t h = video_h_;
  uint64_t x = w * 200;
  if (x < w) {
    // overflow.
    double x = (double)w * 100.0 / (double)h;
    double y = floor(x);
    if (0.5 <= x - y)
      y += 1;
    return y / 100.0;
  }
  uint64_t h_2 = h * 2;
  uint64_t y = (x / h_2) * h_2;
  uint64_t r = y / 2 / h;
  if (h <= x - y) {
    r += 1;
  }
  return (double)r / 100.0;
}

}  // namespace plugins

#endif // HBBTV_ON
