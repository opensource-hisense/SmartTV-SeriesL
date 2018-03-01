// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_CONFIGURATION_CLASS_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_CONFIGURATION_CLASS_H_

#include <map>
#include <string>
#include <vector>
#include "base/memory/weak_ptr.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"

struct PP_Var;

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

struct ConfigurationParam;
class ConfigurationPlugin;

class ConfigurationClass
    : public gin::Wrappable<ConfigurationClass>,
      public gin::NamedPropertyInterceptor {

 public:
  static gin::WrapperInfo kWrapperInfo;
  explicit ConfigurationClass(ConfigurationPlugin* owner,
                                ConfigurationParam *configuration_param);
  ~ConfigurationClass() override;

  static ConfigurationClass* FromV8Object(v8::Isolate* isolate,
                                          v8::Handle<v8::Object> v8_object);

  // Getters // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(v8::Isolate* isolate,
      const std::string &property) override;
  std::vector<std::string> EnumerateNamedProperties(
      v8::Isolate* isolate) override;
  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor();

  v8::Isolate* GetIsolate();
  void OwnerPluginDeleted();

  std::string GetProperty_countryId();
  std::string GetProperty_preferredAudioLanguage();
  std::string GetProperty_preferredSubtitleLanguage();
  std::string GetProperty_preferredUILanguage();
  void toString(gin::Arguments* args);

 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  v8::Local<v8::Value> ResultStringToV8(v8::Isolate* isolate,
                                        std::string result);
  ConfigurationPlugin* owner_;

  std::string preferredAudioLanguage;
  std::string preferredSubtitleLanguage;
  std::string preferredUILanguage;
  std::string countryId;
  bool subtitles_enabled_;
  bool time_shift_synchronized_;

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<ConfigurationClass> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(ConfigurationClass);
};

}  // namespace content
#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_OBJECT_OIPF_CONFIGURATION_CLASS_H_
