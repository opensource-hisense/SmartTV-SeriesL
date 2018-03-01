// Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_OIPF_PLUGIN_OIPF_AVAUDIOCOMPONENT_H_
#define CONTENT_RENDERER_OIPF_PLUGIN_OIPF_AVAUDIOCOMPONENT_H_

#include <stdint.h>
#include <string>
#include <vector>

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

class OipfAvAudioComponent
    : public OipfAvComponent,
      public gin::Wrappable<OipfAvAudioComponent>,
      public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static OipfAvAudioComponent* FromV8Object(
      v8::Isolate* isolate, v8::Handle<v8::Object> v8_object);

  OipfAvAudioComponent(v8::Isolate* isolate, int componentTag,
      int pid, int type, const std::string& encoding, bool encrypted,
      std::string language, bool audioDescription, int audioChannels);

  ~OipfAvAudioComponent() override;

  bool isAudio() override;
  OipfAvAudioComponent* toAudio() override;

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

  std::string language_;
  bool audio_description_;
  int audio_channels_;

  OipfVideoPluginObject* avcontrol_;
  int avcontrol_index_;
  int avcontrol_id_;

  base::WeakPtrFactory<OipfAvAudioComponent> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OipfAvAudioComponent);
};

}  // namespace content

#endif  // CONTENT_RENDERER_OIPF_PLUGIN_OIPF_AVAUDIOCOMPONENT_H_

