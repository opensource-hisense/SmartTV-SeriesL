// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_APPLICATION_PRIVATE_DATA_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_APPLICATION_PRIVATE_DATA_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"
#include "content/renderer/cbip_plugin/cbip_object_owner.h"


namespace IPC {
class Message;
}

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

class CbipObjectOipfApplication;
class CbipObjectOwner;

class CbipObjectOipfApplicationPrivateData
    : public gin::Wrappable<CbipObjectOipfApplicationPrivateData>,
      public gin::NamedPropertyInterceptor,
      public CbipObjectOwner {
 public:
  static gin::WrapperInfo kWrapperInfo;

  // nobody except CbipPluginObjectOipfApplication should call.
  // FIXME: friend?
  explicit CbipObjectOipfApplicationPrivateData(
      CbipObjectOipfApplication* owner);

  ~CbipObjectOipfApplicationPrivateData() override;

  static CbipObjectOipfApplicationPrivateData* FromV8Object(
      v8::Isolate* isolate, v8::Handle<v8::Object> v8_object);

  // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(v8::Isolate* isolate,
                                        const std::string& property) override;
  bool SetNamedProperty(v8::Isolate* isolate,
                        const std::string& property,
                        v8::Local<v8::Value> value) override;
  std::vector<std::string> EnumerateNamedProperties(
      v8::Isolate* isolate) override;

  void ToString(gin::Arguments* args);

  PepperPluginInstanceImpl* instance() override;

  CbipPluginIPC* ipc() override;

  v8::Isolate* GetIsolate() override;

  void OwnerApplicationDeleted();

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor();

 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  void getFreeMem(gin::Arguments* args);

  CbipObjectOipfApplication* owner_;

  // readonly Keyset keyset
  v8::Persistent<v8::Object> keyset_;

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipObjectOipfApplicationPrivateData> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipObjectOipfApplicationPrivateData);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_APPLICATION_PRIVATE_DATA_H_
