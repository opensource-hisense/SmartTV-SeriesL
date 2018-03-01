//Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_OIPF_PLUGIN_OIPF_AVCOMPONENTCOLLECTION_H_
#define CONTENT_RENDERER_OIPF_PLUGIN_OIPF_AVCOMPONENTCOLLECTION_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/renderer/oipf_plugin/oipf_avcomponent.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"
#include "v8/include/v8.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace plugins {
class OipfVideoPlugin;
}  // namespace plugins

namespace content {

class OipfAvComponentCollection
    : public gin::Wrappable<OipfAvComponentCollection>,
      public gin::NamedPropertyInterceptor,
      public gin::IndexedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static OipfAvComponentCollection* FromV8Object(
      v8::Isolate* isolate, v8::Handle<v8::Object> v8_object);

  explicit OipfAvComponentCollection(v8::Isolate* isolate,
      std::vector<OipfAvComponent::AvComponentHandle> components);

  ~OipfAvComponentCollection() override;

  // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(v8::Isolate* isolate,
                                        const std::string& property) override;
  bool SetNamedProperty(v8::Isolate* isolate,
                        const std::string& property,
                        v8::Local<v8::Value> value) override;
  std::vector<std::string> EnumerateNamedProperties(
      v8::Isolate* isolate) override;

  // gin::IndexedPropertyInterceptor
  v8::Local<v8::Value> GetIndexedProperty(v8::Isolate* isolate,
                                          uint32_t index) override;
  bool SetIndexedProperty(v8::Isolate* isolate,
                          uint32_t index,
                          v8::Local<v8::Value> value) override;
  std::vector<uint32_t> EnumerateIndexedProperties(
      v8::Isolate* isolate) override;

 void ToString(gin::Arguments* args);

 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  void Item(gin::Arguments* args);

  std::vector<OipfAvComponent::AvComponentHandle> components_;

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<OipfAvComponentCollection> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OipfAvComponentCollection);
};

}  // namespace content

#endif  // CONTENT_RENDERER_OIPF_PLUGIN_OIPF_AVCOMPONENTCOLLECTION_H_
