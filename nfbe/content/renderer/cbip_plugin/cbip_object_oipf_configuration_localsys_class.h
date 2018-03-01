// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_CONFIGURATION_LOCALSYS_CLASS_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_CONFIGURATION_LOCALSYS_CLASS_H_

#include <map>
#include <string>
#include <vector>
#include "access/content/common/cbip/oipf_common_structs.h"
#include "base/memory/weak_ptr.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"

struct PP_Var;

namespace gin {
class Arguments;
}  // namespace gin

namespace content {
struct ConfigurationLocalSysParam;
class ConfigurationPlugin;

class LocalSystemClass
    : public gin::Wrappable<LocalSystemClass>,
      public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;
  explicit LocalSystemClass(
      ConfigurationPlugin* owner,
      ConfigurationLocalSysParam* configuration_local_sys_param);
  LocalSystemClass();
  ~LocalSystemClass() override;

  static LocalSystemClass* FromV8Object(v8::Isolate* isolate,
                                        v8::Handle<v8::Object> v8_object);

  // Getters // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(v8::Isolate* isolate,
           const std::string &property) override;
  std::vector<std::string>
      EnumerateNamedProperties(v8::Isolate* isolate) override;
  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor();

  v8::Isolate* GetIsolate();
  void OwnerPluginDeleted();

  std::string GetProperty_deviceId();
  std::string GetProperty_modelName();
  std::string GetProperty_vendorName();
  std::string GetProperty_softwareVersion();
  std::string GetProperty_hardwareVersion();
  std::string GetProperty_serialNumber();

 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  v8::Local<v8::Value> ResultStringToV8(v8::Isolate* isolate,
                                        std::string result);
  ConfigurationPlugin* owner_;

  std::string deviceId;
  std::string modelName;
  std::string vendorName;
  std::string softwareVersion;
  std::string hardwareVersion;
  std::string serialNumber;

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<LocalSystemClass> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(LocalSystemClass);
};

}  // namespace content
#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_CONFIGURATION_LOCALSYS_CLASS_H_
