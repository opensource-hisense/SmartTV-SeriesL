#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_JS01_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_JS01_H_

#include <string>

#include "content/renderer/cbip_plugin/cbip_plugin_object.h"

struct PP_Var;

namespace gin {
  class Arguments;
}  // namespace gin

namespace content {

class PepperPluginInstanceImpl;

class CbipPluginObjectJs01 : public CbipPluginObject,
                             public gin::Wrappable<CbipPluginObjectJs01>,
                             public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  ~CbipPluginObjectJs01() override;

  static CbipPluginObjectJs01* FromV8Object(v8::Isolate* isolate,
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

 private:
  CbipPluginObjectJs01(PepperPluginInstanceImpl* instance);

  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  void BuiltinMethod01(gin::Arguments* args);

  PepperPluginInstanceImpl* instance_;

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipPluginObjectJs01> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipPluginObjectJs01);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_JS01_H_
