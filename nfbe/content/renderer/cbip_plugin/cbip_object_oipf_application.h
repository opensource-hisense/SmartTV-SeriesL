#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_APPLICATION_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_APPLICATION_H_

#include <set>
#include <string>

#include "content/renderer/cbip_plugin/cbip_plugin_object.h"
#include "content/renderer/cbip_plugin/cbip_plugin_ipc.h"

namespace IPC {
class Message;
}

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

class CbipPluginObjectOipfApplicationManager;
class RenderFrame;

class CbipObjectOipfApplication
    : public gin::Wrappable<CbipObjectOipfApplication>,
      public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  // nobody except CbipPluginObjectOipfApplicationManager should call.
  // FIXME: friend?
  explicit CbipObjectOipfApplication(
      uint32_t oipf_app_index,
      CbipPluginObjectOipfApplicationManager* oipf_appmgr);

  ~CbipObjectOipfApplication() override;

  static CbipObjectOipfApplication* FromV8Object(
      v8::Isolate* isolate, v8::Handle<v8::Object> v8_object);

  // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(v8::Isolate* isolate,
                                        const std::string& property) override;
  bool SetNamedProperty(v8::Isolate* isolate,
                        const std::string& property,
                        v8::Local<v8::Value> value) override;
  std::vector<std::string> EnumerateNamedProperties(
      v8::Isolate* isolate) override;

  uint32_t app_index() { return oipf_app_index_; };
  
  PepperPluginInstanceImpl* instance();

  CbipPluginIPC* ipc();

  v8::Isolate* GetIsolate();

  // see the below memo about |v8_self|.
  void ApplicationCreated();
  // see the below memo about |v8_self|.
  void ApplicationDeleted();

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor();

  bool OnAppMessageReceived(const IPC::Message& msg);

  // none should call this function.
  void getFreeMem(gin::Arguments* args);

 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  void createApplication(gin::Arguments* args);
  void destroyApplication(gin::Arguments* args);
  void show(gin::Arguments* args);
  void hide(gin::Arguments* args);
  void toString(gin::Arguments* args);

  // a persistent reference to this wrapper object, to prevent GC from
  // disposing this wrapped object.
  // 1. the manager which new()s this wrapped object calls ApplicationCreated(),
  //    which resets v8_self to the wrapper of this wrapped object.
  //    so that, as long as the manager has the raw pointer of this
  //    wrapped object, the corresponding wrapper will not be disposed of.
  // 2. when the manager deletes the application, it calls ApplicationDeleted(),
  //    which resets v8_self to an empty value.
  //    so that, later, after none refers to the wrapper, GC will
  //    dispose of the wrapper and destructs this wrapped object.
  v8::Persistent<v8::Object> v8_self;

  uint32_t oipf_app_index_;

  // readonly ApplicationPrivateData privateData
  v8::Persistent<v8::Object> privateData_;

  // FIXME: decide who manages Application.
  // he must have the isolate and the main world context,
  // and must be able to clean up Applications before the main world
  // context will go away.
  CbipPluginObjectOipfApplicationManager* oipf_appmgr_;

  // if JS has requested of the host to destroy the Application that
  // this Application object refers to (i.e. destroyApplication()),
  // we should not further use this Application object.
  bool is_destroyed_by_app_;

  // if the Application is destroyed by the host (i.e. HOST_APP_DELETED),
  // we should not send further messages to the host; they are unhandled
  // garbages.
  bool is_destroyed_by_host_;

  // if ApplicationLoadError happens or not.
  // this has different semantics from destroy.
  bool load_has_failed_;

  std::set<std::string> is_property_set_;

  class RenderFrameObserverImpl;
  scoped_refptr<RenderFrameObserverImpl> is_renderframe_still_there_;
  void FrameDetached();

  void OnAppLoadError(RenderFrame* render_frame);
  void OnAppShownOrHidden(RenderFrame* render_frame, bool is_shown);

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipObjectOipfApplication> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipObjectOipfApplication);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_APPLICATION_H_
