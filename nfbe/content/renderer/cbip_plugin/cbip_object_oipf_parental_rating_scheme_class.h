// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PARENTAL_RATING_SCHEME_CLASS_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PARENTAL_RATING_SCHEME_CLASS_H_

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

namespace peer {
namespace oipf {
struct ParentalRatingScheme;
}
}

namespace content {

class CbipObjectOipfParentalRatingSchemeClass
    : public gin::Wrappable<CbipObjectOipfParentalRatingSchemeClass>,
      public gin::NamedPropertyInterceptor,
      public CbipObjectOwner {
 public:
  static gin::WrapperInfo kWrapperInfo;

  explicit CbipObjectOipfParentalRatingSchemeClass(
      CbipObjectOwner* owner, const peer::oipf::ParentalRatingScheme& scheme);

  ~CbipObjectOipfParentalRatingSchemeClass() override;

  static CbipObjectOipfParentalRatingSchemeClass* FromV8Object(
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

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor();

  // Return the index of the rating represented by attribute ratingValue inside
  // the parental rating scheme string collection, or -1 if the rating value
  // cannot be found in the collection.
  void indexOf(gin::Arguments* args);

  // Return the URI of the icon representing the rating at index in the rating
  // scheme, or undefined if no item is present at that position.
  // If no icon is available, this method SHALL return null.
  void iconUri(gin::Arguments* args);

  // Return the string representation of the parental rating at index in the
  // parental rating scheme or undefined if no item is present at that position.
  void item(gin::Arguments* args);
 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  CbipObjectOwner* owner_;

  // The unique name that identifies the parental rating scheme
  std::string name_;
  // The parental rating threshold that is currently in use by the OITF’s
  // parental control system for this rating scheme
  v8::Persistent<v8::Object> threshold_;

  // The number of values in the parental rating scheme
  size_t length_;
  std::vector<peer::oipf::ParentalRatingSchemeValue>
     parental_rating_scheme_items;
  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipObjectOipfParentalRatingSchemeClass>
      weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipObjectOipfParentalRatingSchemeClass);
};

}  // namespace content
#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PARENTAL_RATING_SCHEME_CLASS_H_

