//Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_OIPF_PLUGIN_OIPF_VIDEO_PLUGIN_OBJECT_H_
#define CONTENT_RENDERER_OIPF_PLUGIN_OIPF_VIDEO_PLUGIN_OBJECT_H_

#include "base/memory/weak_ptr.h"
#include "content/renderer/oipf_plugin/oipf_avcomponent.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace plugins {
class OipfVideoPlugin;
}  // namespace plugins

namespace content {

class OipfVideoPluginObject
    : public gin::Wrappable<OipfVideoPluginObject>,
      public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static OipfVideoPluginObject* FromV8Object(
      v8::Isolate* isolate, v8::Handle<v8::Object> v8_object);

  // nobody except plugins::OipfVideoPlugin should call.
  // FIXME: friend?
  explicit OipfVideoPluginObject(plugins::OipfVideoPlugin* owner,
                                 v8::Isolate* isolate);

  ~OipfVideoPluginObject() override;

  void shutdown();

  // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(v8::Isolate* isolate,
                                        const std::string& property) override;
  bool SetNamedProperty(v8::Isolate* isolate,
                        const std::string& property,
                        v8::Local<v8::Value> value) override;
  std::vector<std::string> EnumerateNamedProperties(
      v8::Isolate* isolate) override;

  void onTrackAdded(int tlist_idx, int acs_id, int acs_type);
  void onTrackRemoved(int tlist_idx, int acs_id, int acs_type);
  void onTrackChanged(int acs_type);

  double GetAspectRatio(OipfAvVideoComponent* self, int index, int id);

 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  void Play(gin::Arguments* args);
  void Stop(gin::Arguments* args);
  void Seek(gin::Arguments* args);
  void SetFullScreen(gin::Arguments* args);

  void GetComponents(gin::Arguments* args);
  void GetCurrentActiveComponents(gin::Arguments* args);
  void SelectComponent(gin::Arguments* args);
  void UnselectComponent(gin::Arguments* args);
  void SelectComponentInternal(gin::Arguments* args, bool select);

  // helper for GetNamedProperty.
  // do xcall to obtain the value.
  double GetError(v8::Isolate* isolate);
  double GetPlayPosition(v8::Isolate* isolate);
  double GetPlayState(v8::Isolate* isolate);
  double GetSpeed(v8::Isolate* isolate);

  plugins::OipfVideoPlugin* owner_;

  v8::Isolate* isolate_;

  // FIXME: std::array ? because we never do push_back() (at least now).
  std::vector<OipfAvComponent::AvComponentHandle> components_;

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<OipfVideoPluginObject> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OipfVideoPluginObject);
};

}  // namespace content

#endif  // CONTENT_RENDERER_OIPF_PLUGIN_OIPF_VIDEO_PLUGIN_OBJECT_H_
