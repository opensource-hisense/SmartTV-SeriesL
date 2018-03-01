// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_CONFIGURATION_PLUGIN_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_CONFIGURATION_PLUGIN_H_

#include <map>
#include <string>
#include <vector>
#include "content/renderer/cbip_plugin/cbip_plugin_object.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_configuration_class.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_configuration_localsys_class.h"

struct PP_Var;

namespace gin {
class Arguments;
}  // namespace gin

namespace IPC {
class Message;
}

namespace content {

class ConfigurationClass;
class CbipPluginIPC;
class PepperPluginInstanceImpl;

class ConfigurationPlugin
    : public CbipPluginObject,
      public gin::Wrappable<ConfigurationPlugin>,
      public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  ~ConfigurationPlugin() override;

  static ConfigurationPlugin* FromV8Object(
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

  void toString(gin::Arguments* args);

  // Called when the instance is destroyed.
  void InstanceDeleted() override;

  // FIXME: NOTE_0001
  // Called by the following classes to obtain the isolate and the global
  // context, returns NULL if the DOM plugin element of this scriptable object
  // has been deleted.
  // ConfigurationPlugin
  PepperPluginInstanceImpl* instance() { return instance_; }

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor() override;

  v8::Isolate* GetIsolate();

  CbipPluginIPC* ipc() { return ipc_.get(); }

 private:
  explicit ConfigurationPlugin(PepperPluginInstanceImpl* instance);
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  PepperPluginInstanceImpl* instance_;
  // IPC to the peer.
  scoped_refptr<CbipPluginIPC> ipc_;
  // readonly configurationObject
  v8::Persistent<v8::Object> configurationObj;
  // readonly LocalSystemObject
  v8::Persistent<v8::Object> localSystemObj;

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<ConfigurationPlugin> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ConfigurationPlugin);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_CONFIGURATION_PLUGIN_H_
