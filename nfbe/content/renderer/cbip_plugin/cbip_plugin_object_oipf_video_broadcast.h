// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_VIDEO_BROADCAST_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_VIDEO_BROADCAST_H_

#include <map>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "cc/blink/web_layer_impl.h"
#include "cc/layers/layer.h"
#include "content/renderer/cbip_plugin/cbip_object_owner.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"

struct PP_Var;

namespace IPC {
class Message;
}

namespace gin {
class Arguments;
}  // namespace gin

namespace blink {
class WebLayer;
}

namespace peer {
namespace oipf {
struct VideoBroadcastChannel;
}
}

namespace content {

class CbipPluginIPC;

class CbipPluginObjectOipfVideoBroadcast
    : public CbipPluginObject,
      public gin::Wrappable<CbipPluginObjectOipfVideoBroadcast>,
      public gin::NamedPropertyInterceptor,
      public PepperPluginInstanceImplDelegate,
      public CbipObjectOwner {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static uint32_t new_app_index();  // only for testing until IPC is implemented.

  ~CbipPluginObjectOipfVideoBroadcast() override;

  static CbipPluginObjectOipfVideoBroadcast* FromV8Object(
      v8::Isolate* isolate, v8::Handle<v8::Object> v8_object);

  // Allocates a new CbipPluginObject and returns it as a PP_Var with a
  // refcount of 1.
  static PP_Var Create(PepperPluginInstanceImpl* instance);

  // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(v8::Isolate* isolate,
                                        const std::string& property) override;
  bool SetNamedProperty(v8::Isolate* isolate,
                        const std::string& property,
                        v8::Local<v8::Value> value) override;
  std::vector<std::string> EnumerateNamedProperties(
      v8::Isolate* isolate) override;

  // Called when the instance is destroyed.
  void InstanceDeleted() override;

  // FIXME: NOTE_0001
  // Called by the following classes to obtain the isolate and the global context,
  // returns NULL if the DOM plugin element of this scriptable object has been deleted.
  //   CbipObjectOipfApplication
  PepperPluginInstanceImpl* instance() override;

  // FIXME: NOTE_0001
  // Called by the following classes to obtain the IPC message sender.
  //   CbipObjectOipfApplication
  CbipPluginIPC* ipc() override;

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor() override;

  v8::Isolate* GetIsolate() override;

  void DidChangeView(const gfx::Rect& rect) override;
 private:
  CbipPluginObjectOipfVideoBroadcast(PepperPluginInstanceImpl* instance);

  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  void FrameDetached();

  // JavaScript methods
  void getChannelConfig(gin::Arguments* args);
  void bindToCurrentChannel(gin::Arguments* args);
  void createChannelObject(gin::Arguments* args);
  void setChannel(gin::Arguments* args);
  void prevChannel(gin::Arguments* args);
  void nextChannel(gin::Arguments* args);
  void setFullScreen(gin::Arguments* args);
  void setVolume(gin::Arguments* args);
  void getVolume(gin::Arguments* args);
  void release(gin::Arguments* args);
  void stop(gin::Arguments* args);

  void getComponents(gin::Arguments* args);
  void getCurrentActiveComponents(gin::Arguments* args);
  void selectComponent(gin::Arguments* args);
  void unselectComponent(gin::Arguments* args);

  void SetIsFullScreen(bool is_full_screen);

  void OnChannelChangeSucceeded(const peer::oipf::VideoBroadcastChannel& channel);
  void OnChannelChangeError(const peer::oipf::VideoBroadcastChannel& channel, unsigned int error_state);
  void OnPlayStateChange(unsigned int state, bool is_error, int error);
  void OnFullScreenChange();

  // These functions should be called under HandleScope and Context::Scope.
  bool CreateEventObject(std::string event_name, v8::Handle<v8::Object>& event_obj);
  void DispatchEvent(const std::string& event_name, v8::Handle<v8::Object> event_obj);

  PepperPluginInstanceImpl* instance_;

  // IPC to the appmgr.
  scoped_refptr<CbipPluginIPC> ipc_;

  // for application obtained via ApplicationManager.getOwnerApplication(Document document)
  // map from Document to application_index.
  // FIXME: implement.  currently |doc| must be the Document which has this plugin object.
  //std::map<void*,uint32_t> owner_applications_;

  // for application obtained via Application.createApplication(String uri, Boolean createChild)
  // std::set<uint32_t> created_applications_;

  scoped_refptr<cc::Layer> layer_;
  scoped_ptr<blink::WebLayer> web_layer_;
  class RenderFrameObserverImpl;
  scoped_refptr<RenderFrameObserverImpl> observer_;

  v8::Persistent<v8::Object> programmes_;
  v8::Persistent<v8::Object> channel_class_;
  bool is_full_screen_;

  std::set<std::string> is_property_set_;
  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipPluginObjectOipfVideoBroadcast> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(CbipPluginObjectOipfVideoBroadcast);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_APPLICATION_MANAGER_H_
