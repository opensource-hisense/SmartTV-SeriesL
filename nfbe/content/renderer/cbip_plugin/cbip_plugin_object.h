#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"

struct PP_Var;

namespace IPC {
class Message;
}

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

class PepperPluginInstanceImpl;

class CbipPluginObject {
 public:
  static CbipPluginObject* FromInstanceId(int id);

  virtual ~CbipPluginObject();

  static CbipPluginObject* FromV8Object(v8::Isolate* isolate,
                                        v8::Handle<v8::Object> v8_object);

  // Allocates a new CbipPluginObject and returns it as a PP_Var with a
  // refcount of 1.
  static PP_Var Create(PepperPluginInstanceImpl* instance,
                       const std::string& mime_type);

  // Called when the instance is destroyed.
  virtual void InstanceDeleted() = 0;

  virtual gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor() = 0;

  // Called when the IPC thinks |msg| is destined to this instance.
  virtual bool OnAppMgrMessageReceived(const IPC::Message& msg);

 protected:
  CbipPluginObject();
  int cbip_instance_id_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CbipPluginObject);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_H_
