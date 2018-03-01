// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_STRING_COLLECTION_CLASS_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_STRING_COLLECTION_CLASS_H_

#include <string>
#include <vector>

#include "access/peer/include/plugin/dae/video_broadcast_plugin.h"
#include "base/memory/weak_ptr.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

class CbipObjectOwner;

class CbipObjectOipfStringCollectionClass
    : public gin::Wrappable<CbipObjectOipfStringCollectionClass>,
      public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  explicit CbipObjectOipfStringCollectionClass(
      CbipObjectOwner* owner,
      peer::oipf::StringCollectionData string_collection);

  ~CbipObjectOipfStringCollectionClass() override;

  static CbipObjectOipfStringCollectionClass* FromV8Object(
      v8::Isolate* isolate, v8::Handle<v8::Object> v8_object);

  // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(v8::Isolate* isolate,
                                        const std::string& property) override;
  bool SetNamedProperty(v8::Isolate* isolate,
                        const std::string& property,
                        v8::Local<v8::Value> value) override;
  std::vector<std::string> EnumerateNamedProperties(
      v8::Isolate* isolate) override;

  void OwnerDeleted();

  void ToString(gin::Arguments* args);

  void Item(gin::Arguments* args);

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor();

 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  CbipObjectOwner* owner_;

  // length will be always the size of string_collection_
  size_t length_;
  // string collection
  std::vector<std::string> string_collection_;

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipObjectOipfStringCollectionClass> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipObjectOipfStringCollectionClass);
};

}  // namespace content
#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_STRING_COLLECTION_CLASS_H_
