// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PROGRAMME_COLLECTION_CLASS_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PROGRAMME_COLLECTION_CLASS_H_

#include <string>
#include <vector>
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/cbip_plugin/cbip_plugin_ipc.h"
#include "content/renderer/cbip_plugin/cbip_object_owner.h"

#include "base/memory/weak_ptr.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

class CbipObjectOwner;

class CbipObjectOipfProgrammeCollectionClass
    : public gin::Wrappable<CbipObjectOipfProgrammeCollectionClass>,
      public gin::NamedPropertyInterceptor,
      public CbipObjectOwner {
 public:
  static gin::WrapperInfo kWrapperInfo;

  explicit CbipObjectOipfProgrammeCollectionClass(
      CbipObjectOwner* owner);

  ~CbipObjectOipfProgrammeCollectionClass() override;

  static CbipObjectOipfProgrammeCollectionClass* FromV8Object(
      v8::Isolate* isolate, v8::Handle<v8::Object> v8_object);

  // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(v8::Isolate* isolate,
                                        const std::string& property) override;
  bool SetNamedProperty(v8::Isolate* isolate,
                        const std::string& property,
                        v8::Local<v8::Value> value) override;
  std::vector<std::string> EnumerateNamedProperties(
      v8::Isolate* isolate) override;

  PepperPluginInstanceImpl* instance() override;

  CbipPluginIPC* ipc() override;

  v8::Isolate* GetIsolate() override;

  void OwnerDeleted();

  void ToString(gin::Arguments* args);

  void Item(gin::Arguments* args);

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor();

 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  CbipObjectOwner* owner_;

  // Length
  int length_;
  // Array of program class
  v8::Persistent<v8::Object> programmes_;


  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipObjectOipfProgrammeCollectionClass>
      weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipObjectOipfProgrammeCollectionClass);
};

}  // namespace content
#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PROGRAMME_COLLECTION_CLASS_H_
