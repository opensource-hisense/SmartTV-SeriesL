#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_APPLICATION_MANAGER_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_APPLICATION_MANAGER_H_

#include <map>
#include <string>

#include "content/renderer/cbip_plugin/cbip_plugin_object.h"

struct PP_Var;

namespace IPC {
class Message;
}

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

class CbipObjectOipfApplication;
class CbipPluginIPC;
class PepperPluginInstanceImpl;
class RenderFrame;

class CbipPluginObjectOipfApplicationManager
    : public CbipPluginObject,
      public gin::Wrappable<CbipPluginObjectOipfApplicationManager>,
      public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static uint32_t new_app_index();  // only for testing until IPC is implemented.

  ~CbipPluginObjectOipfApplicationManager() override;

  static CbipPluginObjectOipfApplicationManager* FromV8Object(
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
  PepperPluginInstanceImpl* instance() { return instance_; };

  // FIXME: NOTE_0001
  // Called by the following classes to obtain the IPC message sender.
  //   CbipObjectOipfApplication
  CbipPluginIPC* ipc() { return ipc_.get(); };

  // Tracks all non-plugin objects, by using |live_applications_|.
  // void AddOipfApplication(CbipObjectOipfApplication* app_object);
  void RemoveOipfApplication(CbipObjectOipfApplication* app_object);

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor() override;

  // nobody except CbipObjectOipfApplication should call this function.
  CbipObjectOipfApplication* creteApp(uint32_t oipf_app_index);

  // CbipObjectOipfApplication::RenderFrameObserverImpl() calls this function
  // when it receives HbbtvMsg_HOST_APP_LOAD_ERROR from the browser.
  // this function fires "ApplicationLoadError" event.
  void OnAppLoadError(RenderFrame* render_frame,
                      CbipObjectOipfApplication* oipf_app,
                      uint32_t oipf_app_index);

 private:
  CbipPluginObjectOipfApplicationManager(PepperPluginInstanceImpl* instance);

  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  void getOwnerApplication(gin::Arguments* args);
  void toString(gin::Arguments* args);

  PepperPluginInstanceImpl* instance_;

  // IPC to the appmgr.
  scoped_refptr<CbipPluginIPC> ipc_;

  // for application obtained via ApplicationManager.getOwnerApplication(Document document)
  // map from Document to application_index.
  // FIXME: implement.  currently |doc| must be the Document which has this plugin object.
  std::map<void*,uint32_t> owner_applications_;

  // for application obtained via Application.createApplication(String uri, Boolean createChild)
  // std::set<uint32_t> created_applications_;

  // for both |owner_applications_| and |created_applications_|.
  // map from application_index to v8 object.
#define X_MAP_TO_HAVE_V8_PERSISTENT 0
#if X_MAP_TO_HAVE_V8_PERSISTENT
  // FIXME: v8::Persistent<v8::Object> is non copyable.
  // use v8::Persistent<v8::Object,v8::CopyablePersistentTraits<v8::Object>>.
  std::map<uint32_t,v8::Persistent<v8::Object>> live_applications_;
#else
  std::map<uint32_t,CbipObjectOipfApplication*> live_applications_;
#endif

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipPluginObjectOipfApplicationManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipPluginObjectOipfApplicationManager);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_APPLICATION_MANAGER_H_
