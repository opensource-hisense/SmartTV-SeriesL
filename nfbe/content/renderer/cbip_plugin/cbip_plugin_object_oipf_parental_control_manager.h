// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_PARENTAL_CONTROL_MANAGER_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_PARENTAL_CONTROL_MANAGER_H_

#include <string>
#include <vector>
#include "content/renderer/cbip_plugin/cbip_object_owner.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object.h"

struct PP_Var;

namespace gin {
class Arguments;
}  // namespace gin

namespace IPC {
class Message;
}

namespace content {

class CbipPluginIPC;
class PepperPluginInstanceImpl;

class CbipPluginObjectParentalControlManager
    : public CbipPluginObject,
      public gin::Wrappable<CbipPluginObjectParentalControlManager>,
      public gin::NamedPropertyInterceptor,
      public CbipObjectOwner {
 public:
  static gin::WrapperInfo kWrapperInfo;

  ~CbipPluginObjectParentalControlManager() override;

  static CbipPluginObjectParentalControlManager* FromV8Object(
      v8::Isolate* isolate, v8::Handle<v8::Object> v8_object);

  // Allocates a new CbipPluginObjectParentalControlManager and returns
  // it as a PP_Var with a refcount of 1.
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
  // Called by the following classes to obtain
  // the isolate and the global context,
  // returns NULL if the DOM plugin element of this scriptable object
  // has been deleted.
  PepperPluginInstanceImpl* instance() override;

  v8::Isolate* GetIsolate() override;

  CbipPluginIPC* ipc() override;

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor() override;

 private:
  explicit CbipPluginObjectParentalControlManager(
      PepperPluginInstanceImpl* instance);
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  PepperPluginInstanceImpl* instance_;
  // IPC to the peer.
  scoped_refptr<CbipPluginIPC> ipc_;
  // readonly ParentalRatingSchemeCollection
  v8::Persistent<v8::Object> parental_rating_schemes_;

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipPluginObjectParentalControlManager>
    weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipPluginObjectParentalControlManager);
};

}    // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_PARENTAL_CONTROL_MANAGER_H_
