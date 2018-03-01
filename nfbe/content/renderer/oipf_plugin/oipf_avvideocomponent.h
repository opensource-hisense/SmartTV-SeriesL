//Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_OIPF_PLUGIN_OIPF_AVVIDEOCOMPONENT_H_
#define CONTENT_RENDERER_OIPF_PLUGIN_OIPF_AVVIDEOCOMPONENT_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/renderer/oipf_plugin/oipf_avcomponent.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

class OipfVideoPluginObject;

class OipfAvVideoComponent
    : public OipfAvComponent,
      public gin::Wrappable<OipfAvVideoComponent>,
      public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static OipfAvVideoComponent* FromV8Object(
      v8::Isolate* isolate, v8::Handle<v8::Object> v8_object);

  OipfAvVideoComponent(v8::Isolate* isolate, int componentTag,
      int pid, int type, const std::string& encoding, bool encrypted,
      double aspectRatio);

  ~OipfAvVideoComponent() override;

  bool isVideo() override;
  OipfAvVideoComponent* toVideo() override;

  void setAvControl(OipfVideoPluginObject* avcontrol, int avcontrol_index,
                    int avcontrol_id);
  bool getAvControlInfo(OipfAvComponent::AVControlInfo& info);

  void shutdown();

  // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(v8::Isolate* isolate,
                                        const std::string& property) override;
  bool SetNamedProperty(v8::Isolate* isolate, const std::string& property,
                        v8::Local<v8::Value> value) override;
  std::vector<std::string> EnumerateNamedProperties(
      v8::Isolate* isolate) override;

  void ToString(gin::Arguments* args);

  void GetComponentsData(peer::oipf::AVComponentsData *av_components_data);

 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  double aspect_ratio_;

  OipfVideoPluginObject* avcontrol_;
  int avcontrol_index_;
  int avcontrol_id_;

  base::WeakPtrFactory<OipfAvVideoComponent> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OipfAvVideoComponent);
};

}  // namespace content

#endif  // CONTENT_RENDERER_OIPF_PLUGIN_OIPF_AVVIDEOCOMPONENT_H_

