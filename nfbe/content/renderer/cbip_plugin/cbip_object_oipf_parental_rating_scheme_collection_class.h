// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PARENTAL_RATING_SCHEME_COLLECTION_CLASS_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PARENTAL_RATING_SCHEME_COLLECTION_CLASS_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/cbip_plugin/cbip_plugin_ipc.h"
#include "content/renderer/cbip_plugin/cbip_object_owner.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

class CbipObjectOipfParentalRatingSchemeCollectionClass
    : public gin::Wrappable<CbipObjectOipfParentalRatingSchemeCollectionClass>,
      public gin::NamedPropertyInterceptor,
      public CbipObjectOwner {
 public:
  static gin::WrapperInfo kWrapperInfo;

  explicit CbipObjectOipfParentalRatingSchemeCollectionClass(
      CbipObjectOwner* owner);

  ~CbipObjectOipfParentalRatingSchemeCollectionClass() override;

  static CbipObjectOipfParentalRatingSchemeCollectionClass* FromV8Object(
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

  void toString(gin::Arguments* args);

  void item(gin::Arguments* args);

  void getParentalRatingScheme(gin::Arguments* args);

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor();

 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  CbipObjectOwner* owner_;

  // length of ParentalRating collection
  size_t length_;

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipObjectOipfParentalRatingSchemeCollectionClass>
      weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipObjectOipfParentalRatingSchemeCollectionClass);
};

}  // namespace content
#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PARENTAL_RATING_SCHEME_COLLECTION_CLASS_H_

