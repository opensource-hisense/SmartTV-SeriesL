// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_video_broadcast.h"

#include "access/content/common/cbip/oipf_common_structs.h"
#include "access/content/common/cbip/oipf_messages.h"
#include "access/content/renderer/oipf/web_channel_impl.h"
#include "access/peer/include/plugin/dae/video_broadcast_plugin.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "cc/blink/web_layer_impl.h"
#include "cc/layers/layer.h"
#include "cc/layers/solid_color_layer.h"
#include "content/common/cbip/hbbtv_messages.h"
#include "content/public/renderer/render_frame.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_application.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_programme_collection_class.h"
#include "content/renderer/cbip_plugin/cbip_plugin_dispatcher.h"
#include "content/renderer/cbip_plugin/cbip_plugin_ipc.h"
#include "content/renderer/oipf_plugin/oipf_avaudiocomponent.h"
#include "content/renderer/oipf_plugin/oipf_avcomponentcollection.h"
#include "content/renderer/oipf_plugin/oipf_avsubtitlecomponent.h"
#include "content/renderer/oipf_plugin/oipf_avvideocomponent.h"
#include "content/renderer/pepper/message_channel.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/pepper_try_catch.h"
#include "content/renderer/pepper/plugin_module.h"
#include "content/renderer/pepper/v8_var_converter.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "gin/public/gin_embedders.h"
#include "third_party/WebKit/public/web/modules/oipf/WebChannelConfigFactory.h"
#include "third_party/WebKit/public/web/modules/oipf/WebChannelUtil.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"
#include "third_party/skia/include/core/SkColor.h"
#include "v8/include/v8.h"

#define X_LOG_V 0

#define X_TEST_WITHOUT_IPC 0

using ppapi::ScopedPPVar;

namespace content {

namespace {

  /*
    Constants: Media playback extensions constants
    -defined in  OIPF Release 2 Specification under section 7.16.5.1.1
  */
  const char kConstantComponentTypeVideo[] = "COMPONENT_TYPE_VIDEO";
  const char kConstantComponentTypeAudio[] = "COMPONENT_TYPE_AUDIO";
  const char kConstantComponentTypeSutitle[] = "COMPONENT_TYPE_SUBTITLE";

  /*
    Properties
    - Integer width
    - Integer height
    - readonly Boolean fullScreen
    - function onChannelChangeError(Channel channel, Number errorState)
    - readonly Integer playState
    - function onPlayStateChange(Number state, Number error)
    - function onChannelChangeSucceeded(Channel channel)
    - function onFullScreenChange
    - function onfocus
    - function onblur
    - String data
    - readonly Channel currentChannel
  */
const char kPropertyWidth[] = "width";
const char kPropertyHeight[] = "height";
const char kPropertyFullScreen[] = "fullScreen";
const char kPropertyOnChannelChangeError[] = "onChannelChangeError";
const char kPropertyPlayState[] = "playState";
const char kPropertyOnPlayStateChange[] = "onPlayStateChange";
const char kPropertyOnChannelChangeSucceeded[] = "onChannelChangeSucceeded";
const char kPropertyOnFullScreenChange[] = "onFullScreenChange";
const char kPropertyOnfocus[] = "onfocus";
const char kPropertyOnblur[] = "onblur";
const char kPropertyData[] = "data";
const char kPropertyCurrentChannel[] = "currentChannel";
const char kPropertyProgrammes[]  = "programmes";
  /*
    Methods
    - ChannelConfig getChannelConfig()
    - void bindToCurrentChannel()
    - Channel createChannelObject(Integer idType, String dsd, Integer sid)
    - Channel createChannelObject(Integer idType, Integer onid, Integer tsid, Integer sid, Integer sourceID, String ipBroadcastID)
    - void setChannel(Channel channel, Boolean trickplay, String contentAccessDescriptorURL)
    - void prevChannel()
    - void nextChannel()
    - void setFullScreen(Boolean fullscreen)
    - Boolean setVolume(Integer volume)
    - Integer getVolume()
    - void release()
    - void stop()
    Media playback extension methods
    - AVComponentCollection getComponents()
    - AVComponentCollection getCurrentActiveComponents()
    - void selectComponent()
    - void unselectComponent()
  */
const char kMethodGetChannelConfig[] = "getChannelConfig";
const char kMethodBindToCurrentChannel[] = "bindToCurrentChannel";
const char kMethodCreateChannelObject[] = "createChannelObject";
const char kMethodSetChannel[] = "setChannel";
const char kMethodPrevChannel[] = "prevChannel";
const char kMethodNextChannel[] = "nextChannel";
const char kMethodSetFullScreen[] = "setFullScreen";
const char kMethodSetVolume[] = "setVolume";
const char kMethodGetVolume[] = "getVolume";
const char kMethodRelease[] = "release";
const char kMethodStop[] = "stop";
const char kMethodGetComponents[] = "getComponents";
const char kMethodGetCurrentActiveComponents[] = "getCurrentActiveComponents";
const char kMethodSelectComponent[] = "selectComponent";
const char kMethodUnselectComponent[] = "unselectComponent";
  /*
    Events
    - onChannelChangeError
    - onPlayStateChange
    - onChannelChangeSucceeded
    - onFullScreenChange
    - onfocus
    - onblur
  */
const char kEventChannelChangeError[] = "ChannelChangeError";
const char kEventPlayStateChange[] = "PlayStateChange";
const char kEventChannelChangeSucceeded[] = "ChannelChangeSucceeded";
const char kEventFullScreenChange[] = "FullScreenChange";
const char kEventFocus[] = "Focus";
const char kEventBlur[] = "Blur";

const char kGetOwnerApplication[] = "getOwnerApplication";

CbipPluginDispatcher* dispatcher() {
  return RenderThreadImpl::current()->cbip_plugin_dispatcher();
}

v8::Handle<v8::String> CreateV8StringFromString(v8::Isolate* isolate, const std::string string) {
  return v8::String::NewFromUtf8(isolate, string.c_str(), v8::String::kNormalString, string.length());
}
void GetValueFromObject(v8::Isolate* isolate, v8::Handle<v8::Object> target, const std::string key_string, v8::Handle<v8::Value>& result) {
  v8::Handle<v8::Value> key = CreateV8StringFromString(isolate, key_string);
  result = target->Get(key);
}

bool GetObjectFromObject(v8::Isolate* isolate, v8::Handle<v8::Object> target, const std::string key_string, v8::Handle<v8::Object>& result) {
  v8::Local<v8::Value> value;
  GetValueFromObject(isolate, target, key_string, value);
  if (!value->IsObject())
    return false;
  result = v8::Handle<v8::Object>::Cast(value);
  return true;
}

bool GetFunctionFromObject(v8::Isolate* isolate, v8::Handle<v8::Object> target, const std::string key_string, v8::Handle<v8::Function>& result) {
  v8::Local<v8::Value> value;
  GetValueFromObject(isolate, target, key_string, value);
  if (!value->IsFunction()) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast '" << key_string << "' isn't an object.";
    return false;
  }
  result = v8::Handle<v8::Function>::Cast(value);
  return true;
}

bool CallFunction0(v8::Isolate* isolate, blink::WebLocalFrame* frame, v8::Handle<v8::Object> target, std::string function_name, v8::Handle<v8::Value>& result) {
  v8::Local<v8::Function> function;
  if (!GetFunctionFromObject(isolate, target, function_name, function)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast '" << function_name << "' isn't a function object.";
    return false;
  }
  v8::Local<v8::Value> argv[] = {};
  result = frame->callFunctionEvenIfScriptDisabled(function, target, 0, argv);
  return true;
}

bool CallFunction1(v8::Isolate* isolate, blink::WebLocalFrame* frame, v8::Handle<v8::Object> target, std::string function_name, v8::Handle<v8::Value> arg1, v8::Handle<v8::Value>& result) {
  v8::Local<v8::Function> function;
  if (!GetFunctionFromObject(isolate, target, function_name, function)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast '" << function_name << "' isn't a function object.";
    return false;
  }
  v8::Local<v8::Value> argv[] = { arg1 };
  result = frame->callFunctionEvenIfScriptDisabled(function, target, 1, argv);
  return true;
}

bool CallFunction3(v8::Isolate* isolate, blink::WebLocalFrame* frame, v8::Handle<v8::Object> target, std::string function_name, v8::Handle<v8::Value> arg1, v8::Handle<v8::Value> arg2, v8::Handle<v8::Value> arg3, v8::Handle<v8::Value>& result) {
  v8::Local<v8::Function> function;
  if (!GetFunctionFromObject(isolate, target, function_name, function)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast '" << function_name << "' isn't a function object.";
    return false;
  }
  v8::Local<v8::Value> argv[] = { arg1, arg2, arg3 };
  result = frame->callFunctionEvenIfScriptDisabled(function, target, 3, argv);
  return true;
}

bool GetNextIntArgument(gin::Arguments* args, int& result) {
  v8::Handle<v8::Value> arg;
  if (!args->GetNext(&arg))
    return false;
  if (!(arg->IsInt32() || arg->IsUint32()))
    return false;

  v8::Handle<v8::Integer> js_integer = v8::Handle<v8::Integer>::Cast(arg->ToInteger());
  result = js_integer->Value();
  return true;
}

bool GetNextStringArgument(gin::Arguments* args, std::string& result) {
  v8::Handle<v8::Value> arg;
  if (!args->GetNext(&arg))
    return false;
  if (!arg->IsString())
    return false;

  v8::String::Utf8Value js_string(arg->ToString());
  result = std::string(*js_string);
  return true;
}

}  // namespace

// Message from browser.
class CbipPluginObjectOipfVideoBroadcast::RenderFrameObserverImpl
    : public base::RefCounted<CbipPluginObjectOipfVideoBroadcast::RenderFrameObserverImpl>,
      public RenderFrameObserver {
 public:
  explicit RenderFrameObserverImpl(RenderFrame* render_frame,
                                   CbipPluginObjectOipfVideoBroadcast* owner);
  void FrameDetached() override;
  bool OnMessageReceived(const IPC::Message& message) override;
 private:
  void OnChannelChangeSucceeded(const peer::oipf::VideoBroadcastChannel& channel);
  void OnChannelChangeError(const peer::oipf::VideoBroadcastChannel& channel, unsigned int error_state);
  void OnPlayStateChange(unsigned int state, bool is_error, int error);
  void OnFullScreenChange();
  bool is_message_handled_;

  friend base::RefCounted<CbipPluginObjectOipfVideoBroadcast::RenderFrameObserverImpl>;
  friend RenderFrameObserver;
  ~RenderFrameObserverImpl() override;
  RenderFrame* render_frame_;
  CbipPluginObjectOipfVideoBroadcast* owner_;
  bool is_still_attached_;
};

CbipPluginObjectOipfVideoBroadcast::RenderFrameObserverImpl::RenderFrameObserverImpl(
    RenderFrame* render_frame,
    CbipPluginObjectOipfVideoBroadcast* owner)
    : RenderFrameObserver(render_frame),
      is_message_handled_(false),
      render_frame_(render_frame),
      owner_(owner),
      is_still_attached_(true) {
  // FIXME: check: make sure that |render_frame| is still attached.
  // if not, set |is_still_attached_| to false.
}

CbipPluginObjectOipfVideoBroadcast::RenderFrameObserverImpl::~RenderFrameObserverImpl() {
}

void CbipPluginObjectOipfVideoBroadcast::RenderFrameObserverImpl::FrameDetached() {
  render_frame_ = nullptr;
  is_still_attached_ = false;
  owner_->FrameDetached();
}

bool CbipPluginObjectOipfVideoBroadcast::RenderFrameObserverImpl::OnMessageReceived(
    const IPC::Message& message) {
  if (!is_still_attached_)
    return false;
  if (IPC_MESSAGE_CLASS(message) == OipfMsgStart) {
    is_message_handled_ = false;
    bool handled = true;
    IPC_BEGIN_MESSAGE_MAP(CbipPluginObjectOipfVideoBroadcast::RenderFrameObserverImpl, message)
      IPC_MESSAGE_HANDLER(VideoBroadcastHostMsg_Host_ChannelChangeSucceeded,
                          OnChannelChangeSucceeded)
      IPC_MESSAGE_HANDLER(VideoBroadcastHostMsg_Host_ChannelChangeError,
                          OnChannelChangeError)
      IPC_MESSAGE_HANDLER(VideoBroadcastHostMsg_Host_PlayStateChange,
                          OnPlayStateChange)
      IPC_MESSAGE_HANDLER(VideoBroadcastHostMsg_Host_FullScreenChange,
                          OnFullScreenChange)
      IPC_MESSAGE_UNHANDLED(handled = false)
      IPC_END_MESSAGE_MAP()
      if (handled && is_message_handled_) {
        is_message_handled_ = false;
        return true;
      }
    return false;
  }
  return false;
}

void CbipPluginObjectOipfVideoBroadcast::RenderFrameObserverImpl::OnChannelChangeSucceeded(const peer::oipf::VideoBroadcastChannel& channel) {
  owner_->OnChannelChangeSucceeded(channel);
}

void CbipPluginObjectOipfVideoBroadcast::RenderFrameObserverImpl::OnChannelChangeError(const peer::oipf::VideoBroadcastChannel& channel, unsigned int error_state) {
  owner_->OnChannelChangeError(channel, error_state);
}

void CbipPluginObjectOipfVideoBroadcast::RenderFrameObserverImpl::OnPlayStateChange(unsigned int state, bool is_error, int error) {
  owner_->OnPlayStateChange(state, is_error, error);
}

void CbipPluginObjectOipfVideoBroadcast::RenderFrameObserverImpl::OnFullScreenChange() {
  owner_->OnFullScreenChange();
}

CbipPluginObjectOipfVideoBroadcast::~CbipPluginObjectOipfVideoBroadcast() {
  if (instance_) {
    instance_->RemoveCbipPluginObject(this);
    const int routing_id = instance_->render_frame()->GetRoutingID();
    ipc_->Unbind(routing_id);
  }
}

// static
gin::WrapperInfo CbipPluginObjectOipfVideoBroadcast::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
CbipPluginObjectOipfVideoBroadcast* CbipPluginObjectOipfVideoBroadcast::FromV8Object(
    v8::Isolate* isolate, v8::Handle<v8::Object> v8_object) {
  CbipPluginObjectOipfVideoBroadcast* plugin_object;

  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &plugin_object)) {
    return plugin_object;
  }
  return NULL;
}

// static
PP_Var CbipPluginObjectOipfVideoBroadcast::Create(PepperPluginInstanceImpl* instance) {
  V8VarConverter var_converter(instance->pp_instance(),
                               V8VarConverter::kAllowObjectVars);
  PepperTryCatchVar try_catch(instance, &var_converter, NULL);
  CbipPluginObjectOipfVideoBroadcast* plugin_object =
    new CbipPluginObjectOipfVideoBroadcast(instance);

  const PP_Rect pp_rect = instance->view_data().rect;
  gfx::Rect rect(pp_rect.point.x, pp_rect.point.y, pp_rect.size.width, pp_rect.size.height);
  plugin_object->DidChangeView(rect);

  gin::Handle<CbipPluginObjectOipfVideoBroadcast> object =
    gin::CreateHandle(instance->GetIsolate(), plugin_object);
  ScopedPPVar result = try_catch.FromV8(object.ToV8());
  DCHECK(!try_catch.HasException());
  return result.Release();
}

v8::Local<v8::Value> CbipPluginObjectOipfVideoBroadcast::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  {
    VLOG(X_LOG_V) << "ZZZZ CbipPluginObjectOipfVideoBroadcast::GetNamedProperty:"
                  << " " << identifier
      ;
  }

  if (!instance_)
    return v8::Local<v8::Value>();

  auto find = is_property_set_.find(identifier);
  if (find != is_property_set_.end())
    return v8::Local<v8::Value>();

  // Constants
  if (identifier == kConstantComponentTypeVideo) {
    return v8::Integer::New(isolate, 0x0);
  }
  if (identifier == kConstantComponentTypeAudio) {
    return v8::Integer::New(isolate, 0x1);
  }
  if (identifier == kConstantComponentTypeSutitle) {
    return v8::Integer::New(isolate, 0x2);
  }
  // Properties
  if (identifier == kPropertyWidth) {
    blink::WebPluginContainer* container = instance_->container();
    if (!container)
      return v8::Integer::New(isolate, 0);
    blink::WebElement element = container->element();
    const int width = round(element.contentWidth());
    return v8::Integer::New(isolate, width);
  } else if (identifier == kPropertyHeight) {
    blink::WebPluginContainer* container = instance_->container();
    if (!container)
      return v8::Integer::New(isolate, 0);
    blink::WebElement element = container->element();
    const int height = round(element.contentHeight());
    return v8::Integer::New(isolate, height);
  } else if (identifier == kPropertyFullScreen) {
    return v8::Boolean::New(isolate, is_full_screen_);
  } else if (identifier == kPropertyPlayState) {
    const int routing_id = instance_->render_frame()->GetRoutingID();
    int play_state;
    ipc()->GetPlayState(routing_id, &play_state);
    return v8::Integer::New(isolate, play_state);
  } else if (identifier == kPropertyData) {
    return v8::String::Empty(isolate);
  } else if (identifier == kPropertyCurrentChannel) {
    const int routing_id = instance_->render_frame()->GetRoutingID();
    bool is_success;
    peer::oipf::VideoBroadcastChannel channel;
    ipc()->GetCurrentChannel(routing_id, &is_success, &channel);
    if (!is_success)
      return v8::Null(isolate);
    WebChannelImpl* web_channel_impl = new WebChannelImpl(channel);
    v8::Isolate* isolate = instance_->GetIsolate();
    return blink::WebChannelUtil::toV8Value(web_channel_impl, isolate);
  } else if (identifier == kPropertyProgrammes) {
      if (instance_) {
        v8::Isolate* isolate = instance_->GetIsolate();
        return v8::Local<v8::Object>::New(isolate, programmes_);
      }
  }
  /*
    const char kPropertyOnChannelChangeError[] = "onChannelChangeError";
    const char kPropertyOnPlayStateChange[] = "onPlayStateChange";
    const char kPropertyOnChannelChangeSucceeded[] = "onChannelChangeSucceeded";
    const char kPropertyOnFullScreenChange[] = "onFullScreenChange";
    const char kPropertyOnfocus[] = "onfocus";
    const char kPropertyOnblur[] = "onblur";
    const char kPropertyData[] = "data";
  */
  // Methods
  else if (identifier == kMethodGetChannelConfig) {
    // TODO: Maybe, we shouldn't return function after a content sets this
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::getChannelConfig,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kMethodBindToCurrentChannel) {
    // TODO: Maybe, we shouldn't return function after a content sets this.
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::bindToCurrentChannel,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kMethodCreateChannelObject) {
    // TODO: Maybe, we shouldn't return function after a content sets this.
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::createChannelObject,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kMethodSetChannel) {
    // TODO: Maybe, we shouldn't return function after a content sets this.
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::setChannel,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kMethodPrevChannel) {
    // TODO: Maybe, we shouldn't return function after a content sets this.
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::prevChannel,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kMethodNextChannel) {
    // TODO: Maybe, we shouldn't return function after a content sets this.
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::nextChannel,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kMethodSetFullScreen) {
    // TODO: Maybe, we shouldn't return function after a content sets this.
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::setFullScreen,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kMethodRelease) {
    // TODO: Maybe, we shouldn't return function after a content sets this.
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::release,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kMethodStop) {
    // TODO: Maybe, we shouldn't return function after a content sets this.
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::stop,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kMethodGetComponents) {
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::getComponents,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kMethodGetCurrentActiveComponents) {
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::getCurrentActiveComponents,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kMethodSelectComponent) {
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::selectComponent,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kMethodUnselectComponent) {
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::unselectComponent,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

#if BE_ENABLE_HBBTV_ADDONS
  if (identifier == kMethodSetVolume) {
    // TODO: Maybe, we shouldn't return function after a content sets this.
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::setVolume,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kMethodGetVolume) {
    // TODO: Maybe, we shouldn't return function after a content sets this.
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipPluginObjectOipfVideoBroadcast::getVolume,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
#endif
  // TODO: Implement JavaScript

  return v8::Local<v8::Value>();
}

bool CbipPluginObjectOipfVideoBroadcast::SetNamedProperty(v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  {
    VLOG(X_LOG_V) << "ZZZZ CbipPluginObjectOipfVideoBroadcast::GetNamedProperty:"
                  << " " << identifier
      ;
  }

  if (!instance_)
    return false;

  blink::WebPluginContainer* container = instance_->container();
  if (!container)
    return true;
  blink::WebElement element = container->element();
  element.setPreventsDefaultSetter(false);

  // Constants
  if (identifier == kConstantComponentTypeVideo ||
      identifier == kConstantComponentTypeAudio ||
      identifier == kConstantComponentTypeSutitle) {
    return true;
  }
  // Properties
  if (identifier == kPropertyWidth) {
    if (is_full_screen_) {
      element.setPreventsDefaultSetter(true);
      return true;
    }
    v8::Local<v8::String> style_width = value->ToString();
    int width_num = style_width->ToInteger()->Value();
    // element.style.width needs a unit for non-0 value.
    // If string contains unit, ToInteger() returns 0.
    // Therefore, if this is true, value is non-0 string or integer.
    if (width_num != 0) {
      v8::String::Utf8Value width_utf8(style_width);
      const std::string width_str = std::string(*width_utf8);
      style_width = CreateV8StringFromString(isolate, width_str + "px");
    }
    v8::Handle<v8::Object> element_obj = container->v8ObjectForElement();
    v8::Local<v8::Object> style_obj;
    if (!GetObjectFromObject(isolate, element_obj, "style", style_obj))
      return false;
    style_obj->Set(CreateV8StringFromString(isolate, "width"), style_width);
    return false;
  } else if (identifier == kPropertyHeight) {
    if (is_full_screen_) {
      element.setPreventsDefaultSetter(true);
      return true;
    }
    v8::Local<v8::String> style_height = value->ToString();
    int height_num = style_height->ToInteger()->Value();
    // element.style.height needs a unit for non-0 value.
    // If string contains unit, ToInteger() returns 0.
    // Therefore, if this is true, value is non-0 string or integer.
    if (height_num != 0) {
      v8::String::Utf8Value height_utf8(style_height);
      const std::string height_str = std::string(*height_utf8);
      style_height = CreateV8StringFromString(isolate, height_str + "px");
    }
    v8::Handle<v8::Object> element_obj = container->v8ObjectForElement();
    v8::Local<v8::Object> style_obj;
    if (!GetObjectFromObject(isolate, element_obj, "style", style_obj))
      return false;
    style_obj->Set(CreateV8StringFromString(isolate, "height"), style_height);
    return false;
  } else if (identifier == kPropertyFullScreen) {
    // This should be no effect.
    return true;
  } else if (identifier == kPropertyPlayState) {
    // This should be no effect.
    return true;
  } else if (identifier == kPropertyData) {
    // This should be no effect.
    return true;
  } else if (identifier == kPropertyCurrentChannel) {
    // This should be no effect.
    return true;
  } else if (identifier == kPropertyProgrammes) {
    // This should be no effect.
    return true;
  }

  // Contents can set value to methods, so it act as normal case.
  is_property_set_.insert(identifier);
  return false;
}

std::vector<std::string> CbipPluginObjectOipfVideoBroadcast::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  // Constants
  result.push_back(kConstantComponentTypeVideo);
  result.push_back(kConstantComponentTypeAudio);
  result.push_back(kConstantComponentTypeSutitle);
  // Properties
  result.push_back(kPropertyWidth);
  result.push_back(kPropertyHeight);
  result.push_back(kPropertyFullScreen);
  result.push_back(kPropertyPlayState);
  result.push_back(kPropertyData);
  // Methods
  result.push_back(kMethodGetChannelConfig);
  result.push_back(kMethodCreateChannelObject);
  result.push_back(kMethodSetChannel);
  result.push_back(kMethodPrevChannel);
  result.push_back(kMethodNextChannel);
  result.push_back(kMethodSetFullScreen);
  result.push_back(kMethodRelease);
  result.push_back(kMethodStop);
  result.push_back(kMethodGetComponents);
  result.push_back(kMethodGetCurrentActiveComponents);
  result.push_back(kMethodSelectComponent);
  result.push_back(kMethodUnselectComponent);
#if BE_ENABLE_HBBTV_ADDONS
  result.push_back(kMethodSetVolume);
  result.push_back(kMethodGetVolume);
#endif
  // TODO: Implement JavaScript

  return result;
}

void CbipPluginObjectOipfVideoBroadcast::getChannelConfig(gin::Arguments* args) {
  args->Return(blink::WebChannelConfigFactory::createV8Value(instance_->GetIsolate()));
}

void CbipPluginObjectOipfVideoBroadcast::bindToCurrentChannel(gin::Arguments* args) {
  const int routing_id = instance_->render_frame()->GetRoutingID();
  ipc()->BindToCurrentChannel(routing_id);
}

void CbipPluginObjectOipfVideoBroadcast::createChannelObject(gin::Arguments* args) {
  // TODO: Implement
  // This should handle 2 argument types.
  v8::Isolate* isolate = instance_->GetIsolate();

  int id_type;
  if (!GetNextIntArgument(args, id_type)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::createChannelObject() wrong 1st argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }

  v8::Handle<v8::Value> arg2;
  if (!args->GetNext(&arg2)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::createChannelObject() without 2nd argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }
  if (arg2->IsString()) {
    // createChannelObject(Integer idType, String dsd, Integer sid)
    v8::String::Utf8Value js_dsd(arg2->ToString());
    const std::string dsd = std::string(*js_dsd);

    int sid;
    if (!GetNextIntArgument(args, sid)) {
      VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::createChannelObject() wrong 3rd argument.";
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
      return;
    }
    VideoBroadcastCreateChannelObjectSiDirectParam param;
    param.id_type = id_type;
    param.dsd = dsd;
    param.sid = sid;
    const int routing_id = instance_->render_frame()->GetRoutingID();
    bool is_success;
    peer::oipf::VideoBroadcastChannel channel;
    ipc()->CreateChannelObject(routing_id, param, &is_success, &channel);
    if (is_success) {
      WebChannelImpl* web_channel_impl = new WebChannelImpl(channel);
      args->Return(blink::WebChannelUtil::toV8Value(web_channel_impl, isolate));
    } else {
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    }
    return;
  } else if (!(arg2->IsInt32() || arg2->IsUint32())) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::createChannelObject() with wrong type for 2nd argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }
  // createChannelObject(Integer idType, Integer onid, Integer tsid, Integer sid, Integer sourceID, String ipBroadcastID)
  v8::Handle<v8::Integer> js_onid = v8::Handle<v8::Integer>::Cast(arg2->ToInteger());
  int onid = js_onid->Value();

  int tsid;
  if (!GetNextIntArgument(args, tsid)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::createChannelObject() wrong 3rd argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }

  int sid;
  if (!GetNextIntArgument(args, sid)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::createChannelObject() wrong 4th argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }

  int source_id;
  if (!GetNextIntArgument(args, source_id)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::createChannelObject() wrong 5th argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }

  std::string ip_broadcast_id;
  if (!GetNextStringArgument(args, ip_broadcast_id)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::createChannelObject() wrong 6th argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }

  VideoBroadcastCreateChannelObjectOtherParam param;
  param.id_type = id_type;
  param.onid = onid;
  param.tsid = tsid;
  param.sid = sid;
  param.source_id = source_id;
  param.ip_broadcast_id = ip_broadcast_id;
  const int routing_id = instance_->render_frame()->GetRoutingID();
  peer::oipf::VideoBroadcastChannel channel;
  bool is_success;
  ipc()->CreateChannelObject(routing_id, param, &is_success, &channel);
  if (is_success) {
    WebChannelImpl* web_channel_impl = new WebChannelImpl(channel);
    args->Return(blink::WebChannelUtil::toV8Value(web_channel_impl, isolate));
  } else {
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
  }
}

void CbipPluginObjectOipfVideoBroadcast::setChannel(gin::Arguments* args) {
  DCHECK(instance_->GetIsolate() == args->isolate());

  v8::Handle<v8::Value> arg1;
  if (!args->GetNext(&arg1)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::setChannel() without 1st argument.";
    return;
  }
  if (!arg1->IsObject()) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::setChannel() with wrong type for 1st argument.";
    return;
  }
  WebChannelImpl* web_channel_impl =
      static_cast<WebChannelImpl*>(blink::WebChannelUtil::fromV8Value(arg1));
  if (!web_channel_impl) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::setChannel() with wrong object for 1st argument.";
    return;
  }
  peer::oipf::VideoBroadcastChannel channel = web_channel_impl->GetVideoBroadcastChannel();

  v8::Handle<v8::Value> arg2;
  if (!args->GetNext(&arg2)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::setChannel() without 2nd argument.";
    return;
  }
  if (!arg2->IsBoolean()) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::setChannel() with wrong type for 2nd argument.";
    return;
  }
  v8::Handle<v8::Boolean> js_trickplay = v8::Handle<v8::Boolean>::Cast(arg2->ToBoolean());
  const bool trickplay = js_trickplay->Value();

  std::string url;
  v8::Handle<v8::Value> arg3;
  if (args->GetNext(&arg3)) {
    if (arg3->IsString()) {
      v8::String::Utf8Value js_url(arg3->ToString());
      url = std::string(*js_url);
    }
  }
  const int routing_id = instance_->render_frame()->GetRoutingID();

  ipc()->SetChannel(routing_id, channel, trickplay, url);
}

void CbipPluginObjectOipfVideoBroadcast::prevChannel(gin::Arguments* args) {
  const int routing_id = instance_->render_frame()->GetRoutingID();
  ipc()->PrevChannel(routing_id);
}

void CbipPluginObjectOipfVideoBroadcast::nextChannel(gin::Arguments* args) {
  const int routing_id = instance_->render_frame()->GetRoutingID();
  VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::nextChannel: routing_id: " << routing_id << ", this: " << (void*)this << ", observer: " << (void*)observer_.get();
  ipc()->NextChannel(routing_id);
}

// void setFullScreen(Boolean fullscreen)
void CbipPluginObjectOipfVideoBroadcast::setFullScreen(gin::Arguments* args) {
  v8::Handle<v8::Value> arg1;
  if (!args->GetNext(&arg1)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::setFullScreen() without argument.";
    return;
  }
  // TODO: Should I check 2 or more argument case?
  // I'm not sure that this conversion is correct.
  v8::Handle<v8::Boolean> fullscreen = v8::Handle<v8::Boolean>::Cast(arg1->ToBoolean());
  bool is_full_screen = fullscreen->Value();
  SetIsFullScreen(is_full_screen);
}

void CbipPluginObjectOipfVideoBroadcast::setVolume(gin::Arguments* args) {
  DCHECK(instance_->GetIsolate() == args->isolate());
  v8::Handle<v8::Value> arg1;
  if (!args->GetNext(&arg1)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::setVolume() without argument.";
    args->Return(v8::Local<v8::Value>(v8::Boolean::New(instance_->GetIsolate(), false)));
    return;
  }
  if (!(arg1->IsInt32() || arg1->IsUint32())) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::setVolume() with wrong type.";
    args->Return(v8::Local<v8::Value>(v8::Boolean::New(instance_->GetIsolate(), false)));
    return;
  }
  v8::Handle<v8::Integer> js_volume = v8::Handle<v8::Integer>::Cast(arg1->ToInteger());
  const int volume = js_volume->Value();
  if (volume < 0 || volume > 100) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::setVolume() with wrong value.";
    args->Return(v8::Local<v8::Value>(v8::Boolean::New(instance_->GetIsolate(), false)));
    return;
  }
  const int routing_id = instance_->render_frame()->GetRoutingID();
  ipc()->SetVolume(routing_id, volume);
  args->Return(v8::Local<v8::Value>(v8::Boolean::New(instance_->GetIsolate(), true)));
  VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::setVolume(): " << volume;
}

void CbipPluginObjectOipfVideoBroadcast::getVolume(gin::Arguments* args) {
  DCHECK(instance_->GetIsolate() == args->isolate());
  // TODO: Get from peer
  const int routing_id = instance_->render_frame()->GetRoutingID();
  int volume;
  ipc()->GetVolume(routing_id, &volume);
  v8::Handle<v8::Value> result = v8::Integer::New(instance_->GetIsolate(), volume);
  args->Return(result);
  VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::getVolume(): " << volume;
}

void CbipPluginObjectOipfVideoBroadcast::release(gin::Arguments* args) {
  const int routing_id = instance_->render_frame()->GetRoutingID();
  ipc()->VideoBroadcastRelease(routing_id);
}

void CbipPluginObjectOipfVideoBroadcast::stop(gin::Arguments* args) {
  const int routing_id = instance_->render_frame()->GetRoutingID();
  ipc()->Stop(routing_id);
}

void CbipPluginObjectOipfVideoBroadcast::getComponents(gin::Arguments* args) {
  v8::Isolate* isolate =  instance_->GetIsolate();

  v8::Handle<v8::Value> arg1;
  int component_type = -1;//default type to get all components
  if (!args->GetNext(&arg1)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::getComponents without argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }
  if (arg1->IsInt32()) {
    v8::Handle<v8::Integer> js_component_type = v8::Handle<v8::Integer>::Cast(arg1->ToInteger());
    component_type = js_component_type->Value();
    if (component_type < peer::oipf::COMPONENT_TYPE_VIDEO ||
        component_type > peer::oipf::COMPONENT_TYPE_SUBTITLE) {
      VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::getComponents passed with Invalid component type.";
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
      return;
    }
  } else if ((!arg1->IsUndefined()) && (!arg1->IsNull())) {
      VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::getComponents passed with Invalid argument.";
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
      return;
  }
  const int routing_id = instance_->render_frame()->GetRoutingID();
  bool is_success;
  peer::oipf::AVComponentCollection av_component_collection;
  ipc()->GetComponents(routing_id, component_type, &is_success, &av_component_collection);
  if (!is_success) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::getComponents Fails.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }
  std::vector<OipfAvComponent::AvComponentHandle> avcomponents;
  for (auto i = 0; i < av_component_collection.size(); i++) {
    peer::oipf::AVComponentsData component  = av_component_collection.at(i);

    if (component.type == peer::oipf::COMPONENT_TYPE_VIDEO) {
      OipfAvVideoComponent* video_component = new OipfAvVideoComponent(
          isolate, component.component_tag, component.pid, OipfAvComponent::kTypeVideo,
          component.encoding, component.encrypted, component.aspect_ratio);
      v8::Local<v8::Object> js_object = video_component->GetWrapper(isolate);
      OipfAvComponent::AvComponentHandle data(isolate, js_object);
      avcomponents.push_back(data);
    }
    if (component.type == peer::oipf::COMPONENT_TYPE_AUDIO) {
      OipfAvAudioComponent* audio_component = new OipfAvAudioComponent(isolate,
          component.component_tag, component.pid, OipfAvComponent::kTypeAudio,
          component.encoding, component.encrypted, component.audio_language,
          component.audio_description, component.audio_channels);
      v8::Local<v8::Object> js_object = audio_component->GetWrapper(isolate);
      OipfAvComponent::AvComponentHandle data(isolate, js_object);
      avcomponents.push_back(data);
    }
    if (component.type == peer::oipf::COMPONENT_TYPE_SUBTITLE) {
      OipfAvSubtitleComponent* subtitle_component = new OipfAvSubtitleComponent(isolate,
          component.component_tag, component.pid, OipfAvComponent::kTypeSubtitle,
          component.encoding, component.encrypted, component.subtitle_language,
          component.hearing_impaired);
      v8::Local<v8::Object> js_object = subtitle_component->GetWrapper(isolate);
      OipfAvComponent::AvComponentHandle data(isolate, js_object);
      avcomponents.push_back(data);
    }
  }
  OipfAvComponentCollection* result = new OipfAvComponentCollection(isolate, avcomponents);
  args->Return(result->GetWrapper(isolate));
}

void CbipPluginObjectOipfVideoBroadcast::getCurrentActiveComponents(gin::Arguments* args) {
  v8::Isolate* isolate = instance_->GetIsolate();
  v8::Handle<v8::Value> arg1;
  int component_type = -1;//default type to get all components
  if (!args->GetNext(&arg1)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::getCurrentActiveComponents without argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
    return;
  }
  if (!arg1->IsInt32()) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::getCurrentActiveComponents passed with Invalid argument.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
    return;
  }
  v8::Handle<v8::Integer> js_component_type = v8::Handle<v8::Integer>::Cast(arg1->ToInteger());
  component_type = js_component_type->Value();
  if (component_type < peer::oipf::COMPONENT_TYPE_VIDEO ||
      component_type > peer::oipf::COMPONENT_TYPE_SUBTITLE) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::getCurrentActiveComponents passed with Invalid component type.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Undefined(isolate)));
    return;
  }

  const int routing_id = instance_->render_frame()->GetRoutingID();
  bool is_success;
  peer::oipf::AVComponentCollection av_component_collection;
  ipc()->GetCurrentActiveComponents(routing_id, component_type, &is_success, &av_component_collection);
  if (!is_success) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::getCurrentActiveComponents Fails.";
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(isolate)));
    return;
  }
  std::vector<OipfAvComponent::AvComponentHandle> avcomponents;
  for (auto component : av_component_collection) {
    if (component.type == peer::oipf::COMPONENT_TYPE_VIDEO) {
      OipfAvVideoComponent* video_component =
          new OipfAvVideoComponent(isolate, component.component_tag, component.pid,
                                   OipfAvComponent::kTypeVideo, component.encoding,
                                   component.encrypted, component.aspect_ratio);
      v8::Local<v8::Object> js_object = video_component->GetWrapper(isolate);
      OipfAvComponent::AvComponentHandle data(isolate, js_object);
      avcomponents.push_back(data);
    }
    if (component.type == peer::oipf::COMPONENT_TYPE_AUDIO) {
      OipfAvAudioComponent* audio_component =
          new OipfAvAudioComponent(isolate, component.component_tag, component.pid,
                                   OipfAvComponent::kTypeAudio, component.encoding,
                                   component.encrypted, component.audio_language,
                                   component.audio_description, component.audio_channels);
      v8::Local<v8::Object> js_object = audio_component->GetWrapper(isolate);
      OipfAvComponent::AvComponentHandle data(isolate, js_object);
      avcomponents.push_back(data);
    }
    if (component.type == peer::oipf::COMPONENT_TYPE_SUBTITLE) {
      OipfAvSubtitleComponent* subtitle_component =
          new OipfAvSubtitleComponent(isolate, component.component_tag, component.pid,
                                      OipfAvComponent::kTypeSubtitle, component.encoding,
                                      component.encrypted, component.subtitle_language,
                                      component.hearing_impaired);
      v8::Local<v8::Object> js_object = subtitle_component->GetWrapper(isolate);
      OipfAvComponent::AvComponentHandle data(isolate, js_object);
      avcomponents.push_back(data);
    }
  }
  OipfAvComponentCollection* result = new OipfAvComponentCollection(isolate, avcomponents);
  args->Return(result->GetWrapper(isolate));
}

void CbipPluginObjectOipfVideoBroadcast::selectComponent(gin::Arguments* args){
  v8::Isolate* isolate = instance_->GetIsolate();
  const int routing_id = instance_->render_frame()->GetRoutingID();
  v8::Handle<v8::Value> arg1;
  int component_type = -1; // for detecting the type of selectComponent() call in the tv tuner binder code
  peer::oipf::AVComponentsData av_components_data;

  if (!args->GetNext(&arg1)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::selectComponent without argument.";
    return;
  }
  if (arg1->IsInt32()) {
    v8::Handle<v8::Integer> js_component_type = v8::Handle<v8::Integer>::Cast(arg1->ToInteger());
    component_type = js_component_type->Value();
    if (component_type < peer::oipf::COMPONENT_TYPE_VIDEO ||
        component_type > peer::oipf::COMPONENT_TYPE_SUBTITLE) {
      VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::selectComponent passed with invalid component type.";
      return;
    }
    ipc()->SelectComponent(routing_id, component_type, nullptr);
  } else if (arg1->IsObject()) {
    v8::Handle<v8::Object> component_obj = v8::Handle<v8::Object>::Cast(arg1);

    OipfAvSubtitleComponent* subtitle_component = OipfAvSubtitleComponent::FromV8Object(isolate, component_obj);
    OipfAvAudioComponent* audio_component = OipfAvAudioComponent::FromV8Object(isolate, component_obj);
    OipfAvVideoComponent* video_component = OipfAvVideoComponent::FromV8Object(isolate, component_obj);
    if (subtitle_component) {
      subtitle_component->GetComponentsData(&av_components_data);
      ipc()->SelectComponent(routing_id, component_type, &av_components_data);
    } else if (video_component) {
      video_component->GetComponentsData(&av_components_data);
      ipc()->SelectComponent(routing_id, component_type, &av_components_data);
    } else if (audio_component) {
      audio_component->GetComponentsData(&av_components_data);
      ipc()->SelectComponent(routing_id, component_type, &av_components_data);
    }
  }
}

void CbipPluginObjectOipfVideoBroadcast::unselectComponent(gin::Arguments* args) {
  v8::Isolate* isolate = instance_->GetIsolate();
  const int routing_id = instance_->render_frame()->GetRoutingID();
  v8::Handle<v8::Value> arg1;
  int component_type = -1; // for detecting the type of selectComponent() call in the tv tuner binder code
  peer::oipf::AVComponentsData av_components_data;

  if (!args->GetNext(&arg1)) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::unselectComponent without argument.";
    return;
  }
  if (arg1->IsInt32()) {
    v8::Handle<v8::Integer> js_component_type = v8::Handle<v8::Integer>::Cast(arg1->ToInteger());
    component_type = js_component_type->Value();
    if (component_type < peer::oipf::COMPONENT_TYPE_VIDEO ||
      component_type > peer::oipf::COMPONENT_TYPE_SUBTITLE) {
      VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::unselectComponent passed with invalid component type.";
      return;
    }
    ipc()->UnselectComponent(routing_id, component_type, nullptr);
  } else if (arg1->IsObject()) {
    v8::Handle<v8::Object> component_obj = v8::Handle<v8::Object>::Cast(arg1);

    OipfAvSubtitleComponent* subtitle_component = OipfAvSubtitleComponent::FromV8Object(isolate, component_obj);
    OipfAvAudioComponent* audio_component = OipfAvAudioComponent::FromV8Object(isolate, component_obj);
    OipfAvVideoComponent* video_component = OipfAvVideoComponent::FromV8Object(isolate, component_obj);
    if (subtitle_component) {
      subtitle_component->GetComponentsData(&av_components_data);
      ipc()->UnselectComponent(routing_id, component_type, &av_components_data);
    } else if (video_component) {
      video_component->GetComponentsData(&av_components_data);
      ipc()->UnselectComponent(routing_id, component_type, &av_components_data);
    } else if (audio_component) {
      audio_component->GetComponentsData(&av_components_data);
      ipc()->UnselectComponent(routing_id, component_type, &av_components_data);
    }
  }
}

void CbipPluginObjectOipfVideoBroadcast::SetIsFullScreen(bool is_full_screen) {
  if (is_full_screen_ == is_full_screen)
    return;
  is_full_screen_ = is_full_screen;
  const int routing_id = instance_->render_frame()->GetRoutingID();
  ipc()->SetFullScreen(routing_id, is_full_screen_);
  VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::SetIsFullScreen(): " << is_full_screen_;

  if (!instance_)
    return;

  RenderFrame* render_frame = instance_->render_frame();
  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  blink::WebDocument document = frame->document();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (!isolate)
    return;

  blink::WebPluginContainer* container = instance_->container();
  if (!container)
    return;

  if (is_full_screen_) {
    v8::Handle<v8::Object> element_obj = container->v8ObjectForElement();
    v8::Local<v8::Value> result;
    CallFunction0(isolate, frame, element_obj, "webkitRequestFullscreen", result);
  } else {
    v8::Local<v8::Object> window_obj = frame->mainWorldScriptContext()->Global();
    v8::Handle<v8::Object> document_obj;
    if (!GetObjectFromObject(isolate, window_obj, "document", document_obj))
      return;
    v8::Local<v8::Value> result;
    CallFunction0(isolate, frame, document_obj, "webkitExitFullscreen", result);
  }
}

void CbipPluginObjectOipfVideoBroadcast::OnChannelChangeSucceeded(const peer::oipf::VideoBroadcastChannel& channel) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (!isolate)
    return;
  v8::HandleScope handle_scope(isolate);
  RenderFrame* render_frame = instance_->render_frame();
  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> event_obj;
  if (!CreateEventObject(kEventChannelChangeSucceeded, event_obj))
    return;

  WebChannelImpl* web_channel_impl = new WebChannelImpl(channel);
  event_obj->Set(CreateV8StringFromString(isolate, "channel"),
                 blink::WebChannelUtil::toV8Value(web_channel_impl, isolate));

  DispatchEvent(kEventChannelChangeSucceeded, event_obj);
}

void CbipPluginObjectOipfVideoBroadcast::OnChannelChangeError(const peer::oipf::VideoBroadcastChannel& channel, unsigned int error_state) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (!isolate)
    return;
  v8::HandleScope handle_scope(isolate);
  RenderFrame* render_frame = instance_->render_frame();
  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> event_obj;
  if (!CreateEventObject(kEventChannelChangeError, event_obj))
    return;

  WebChannelImpl* web_channel_impl = new WebChannelImpl(channel);
  event_obj->Set(CreateV8StringFromString(isolate, "channel"),
                 blink::WebChannelUtil::toV8Value(web_channel_impl, isolate));
  event_obj->Set(CreateV8StringFromString(isolate, "errorState"), v8::Local<v8::Integer>(v8::Integer::New(isolate, error_state)));

  DispatchEvent(kEventChannelChangeError, event_obj);
}

void CbipPluginObjectOipfVideoBroadcast::OnPlayStateChange(unsigned int state, bool is_error, int error) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (!isolate)
    return;
  v8::HandleScope handle_scope(isolate);
  RenderFrame* render_frame = instance_->render_frame();
  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> event_obj;
  if (!CreateEventObject(kEventPlayStateChange, event_obj))
    return;

  event_obj->Set(CreateV8StringFromString(isolate, "state"), v8::Local<v8::Integer>(v8::Integer::New(isolate, state)));
  if (is_error)
    event_obj->Set(CreateV8StringFromString(isolate, "error"), v8::Local<v8::Integer>(v8::Integer::New(isolate, error)));

  DispatchEvent(kEventPlayStateChange, event_obj);
}

void CbipPluginObjectOipfVideoBroadcast::OnFullScreenChange() {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (!isolate)
    return;
  v8::HandleScope handle_scope(isolate);
  RenderFrame* render_frame = instance_->render_frame();
  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> event_obj;
  if (!CreateEventObject(kEventFullScreenChange, event_obj))
    return;

  DispatchEvent(kEventFullScreenChange, event_obj);
}

void CbipPluginObjectOipfVideoBroadcast::InstanceDeleted() {
  const int routing_id = instance_->render_frame()->GetRoutingID();
  ipc_->Unbind(routing_id);
  instance_ = nullptr;
  observer_ = nullptr;
}

CbipPluginObjectOipfVideoBroadcast::CbipPluginObjectOipfVideoBroadcast(
    PepperPluginInstanceImpl* instance)
    : gin::NamedPropertyInterceptor(instance->GetIsolate(), this),
      instance_(instance),
      ipc_(dispatcher()->GetAppMgr(cbip_instance_id_)), // TODO(yuki): IPC?
      is_full_screen_(false),
      weak_ptr_factory_(this) {
  instance_->SetDelegate(this);
  layer_ = cc::SolidColorLayer::Create();
  layer_->SetIsDrawable(true);
  layer_->SetBlendMode(SkXfermode::kSrc_Mode);
  // Test code which draw video area from Chromium.
  // layer_->SetBackgroundColor(SkColorSetARGB(128, 255, 0, 0));
  gfx::Size size(10, 10);
  layer_->SetBounds(size);
  web_layer_.reset(new cc_blink::WebLayerImpl(layer_));
  blink::WebPluginContainer* container = instance_->container();
  if (container) {
    container->setWebLayer(web_layer_.get());
  } else {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast no container!";
  }
  v8::Isolate* isolate = instance->GetIsolate();
  instance_->AddCbipPluginObject(this);
  RenderFrameObserverImpl* impl = new RenderFrameObserverImpl(instance_->render_frame(), this);
  VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast new observer!" << (void*)this << ", " << impl << " for " << (void*)instance_->render_frame();
  observer_ = impl;
  const int routing_id = instance_->render_frame()->GetRoutingID();
  ipc_->Bind(routing_id);

  CbipObjectOipfProgrammeCollectionClass* programmes =
      new CbipObjectOipfProgrammeCollectionClass(this);
  programmes_.Reset(instance_->GetIsolate(), programmes->GetWrapper(instance_->GetIsolate()));
}

void CbipPluginObjectOipfVideoBroadcast::FrameDetached() {
  observer_ = nullptr;
}

gin::ObjectTemplateBuilder CbipPluginObjectOipfVideoBroadcast::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<CbipPluginObjectOipfVideoBroadcast>::GetObjectTemplateBuilder(isolate)
    .AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor* CbipPluginObjectOipfVideoBroadcast::asGinNamedPropertyInterceptor() {
  return this;
}

// TODO: JS C++ implementation

bool CbipPluginObjectOipfVideoBroadcast::CreateEventObject(std::string event_name, v8::Handle<v8::Object>& event_obj) {
  if (!instance_)
    return false;

  RenderFrame* render_frame = instance_->render_frame();
  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  blink::WebDocument document = frame->document();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (!isolate)
    return false;

  v8::Local<v8::Context> context = frame->mainWorldScriptContext();

  blink::WebPluginContainer* container = instance_->container();
  if (!container)
    return false;
  //v8::Local<v8::Object> element_obj = GetWrapper(isolate);
  v8::Handle<v8::Object> element_obj = container->v8ObjectForElement();
  v8::Local<v8::Object> window_obj = context->Global();

  // FIXME: check: is there any way to obtain the blink wrapper out of |document| ?

  v8::Handle<v8::Object> document_obj;
  if (!GetObjectFromObject(isolate, window_obj, "document", document_obj))
    return false;

  v8::Local<v8::Value> result;
  if (!CallFunction1(isolate, frame, document_obj, "createEvent", CreateV8StringFromString(isolate, "Event"), result))
    return false;
  if (!result->IsObject()) {
    VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast " << __LINE__
                  << "'window.document.createEvent' 'Event' failed";
    return false;
  }
  event_obj = v8::Handle<v8::Object>::Cast(result);

  if (!CallFunction3(isolate, frame, event_obj, "initEvent",
                     CreateV8StringFromString(isolate, event_name),
                     v8::Boolean::New(isolate, false), v8::Boolean::New(isolate, false),
                     result))
    return false;
  return true;
}

void CbipPluginObjectOipfVideoBroadcast::DispatchEvent(const std::string& event_name, v8::Handle<v8::Object> event_obj) {
  if (!instance_)
    return;

  RenderFrame* render_frame = instance_->render_frame();
  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  blink::WebDocument document = frame->document();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (!isolate)
    return;

  blink::WebPluginContainer* container = instance_->container();
  if (!container)
    return;
  //v8::Local<v8::Object> element_obj = GetWrapper(isolate);
  v8::Handle<v8::Object> element_obj = container->v8ObjectForElement();

  // FIXME: can we reuse the event object?  if we can, how?
  // e.g. the case when DOM2 handler alters the event object, is this visible to
  // DOM0 handler, or vice versa.

  v8::Local<v8::Value> result;
  /* DOM2 */
  CallFunction1(isolate, frame, element_obj, "dispatchEvent", event_obj, result);

  /* DOM0 */
  std::string callback_name("on");
  callback_name += event_name;
  CallFunction1(isolate, frame, element_obj, callback_name, event_obj, result);
}


PepperPluginInstanceImpl* CbipPluginObjectOipfVideoBroadcast::instance() {
  if (instance_)
    return instance_;
  return nullptr;
}

CbipPluginIPC* CbipPluginObjectOipfVideoBroadcast::ipc() {
  if(ipc_)
    return ipc_.get();
  return nullptr;
}

v8::Isolate* CbipPluginObjectOipfVideoBroadcast::GetIsolate() {
  if (instance_)
    return instance_->GetIsolate();
  return 0;
}

void
CbipPluginObjectOipfVideoBroadcast::DidChangeView(const gfx::Rect& rect) {
  const int routing_id = instance_->render_frame()->GetRoutingID();
  ipc()->DidChangeView(routing_id, rect);
  // TODO: Send this to peer
  VLOG(X_LOG_V) << "CbipPluginObjectOipfVideoBroadcast::DidChangeView(" << rect.x() << ", " << rect.y() << ", " << rect.width() << ", " << rect.height() << ")";
}


}  // namespace content
