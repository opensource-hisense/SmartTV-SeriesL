// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_CAPABILITIES_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_CAPABILITIES_H_

#include <set>
#include <string>
#include <vector>
#include "content/renderer/cbip_plugin/cbip_plugin_object.h"

struct PP_Var;

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

class CbipPluginIPC;
class PepperPluginInstanceImpl;

class CbipPluginObjectCapabilities
    : public CbipPluginObject,
      public gin::Wrappable<CbipPluginObjectCapabilities>,
      public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  ~CbipPluginObjectCapabilities() override;

  static CbipPluginObjectCapabilities* FromV8Object(
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
  // CbipPluginObjectCapabilities
  PepperPluginInstanceImpl* instance() { return instance_; }

  gin::NamedPropertyInterceptor*
    asGinNamedPropertyInterceptor() override;

  v8::Isolate* GetIsolate();

  CbipPluginIPC* ipc() { return ipc_.get(); }

  // Check if the OITF supports the passed capability.
  // Returns true if the OITF supports the passed capability, false otherwise.
  void hasCapability(gin::Arguments* args);

 private:
  explicit CbipPluginObjectCapabilities(PepperPluginInstanceImpl* instance);
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  PepperPluginInstanceImpl* instance_;
  // IPC to the peer.
  scoped_refptr<CbipPluginIPC> ipc_;

  std::set<std::string> is_property_set_;
  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipPluginObjectCapabilities> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipPluginObjectCapabilities);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_CAPABILITIES_H_
