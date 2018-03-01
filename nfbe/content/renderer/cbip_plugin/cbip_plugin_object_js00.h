#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_JS00_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_JS00_H_

#include <string>

#include "content/renderer/cbip_plugin/cbip_plugin_object.h"

struct CbipJs00Msg_PT1_Params;
struct PP_Var;

namespace IPC {
class Message;
}

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

class CbipPluginIPC;
class PepperPluginInstanceImpl;

class CbipPluginObjectJs00 : public CbipPluginObject,
                             public gin::Wrappable<CbipPluginObjectJs00>,
                             public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  ~CbipPluginObjectJs00() override;

  static CbipPluginObjectJs00* FromV8Object(v8::Isolate* isolate,
                                            v8::Handle<v8::Object> v8_object);

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

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor() override;

  bool OnAppMgrMessageReceived(const IPC::Message& msg) override;

 private:
  CbipPluginObjectJs00(PepperPluginInstanceImpl* instance);

  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  void BuiltinMethod00(gin::Arguments* args);
  void BuiltinMethod02(gin::Arguments* args);
  void BuiltinMethod03(gin::Arguments* args);
  void BuiltinMethod04(gin::Arguments* args);

  // IPC message handlers used by OnAppMgrMessageReceived()
  void OnAsyncEvent1(const CbipJs00Msg_PT1_Params& params);

  PepperPluginInstanceImpl* instance_;

  // IPC to the appmgr.
  scoped_refptr<CbipPluginIPC> ipc_;

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipPluginObjectJs00> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipPluginObjectJs00);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_JS00_H_
