// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PARENTAL_RATING_CLASS_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PARENTAL_RATING_CLASS_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"
#include "content/renderer/cbip_plugin/cbip_object_owner.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

class CbipObjectOwner;
class CbipObjectOipfProgrammeCollectionClass;

class CbipObjectOipfParentalRatingClass
    : public gin::Wrappable<CbipObjectOipfParentalRatingClass>,
      public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  explicit CbipObjectOipfParentalRatingClass(
      CbipObjectOwner* owner,
      const peer::oipf::ParentalRatingItem& parental_rating);

  ~CbipObjectOipfParentalRatingClass() override;

  static CbipObjectOipfParentalRatingClass* FromV8Object(
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

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor();

  // This api is used for browser unittest to get ParentalRating Class
  // properties values into ParentalRatingItem Struct
  static void TestConvertToStruct(CbipObjectOipfParentalRatingClass* p_value,
      peer::oipf::ParentalRatingItem *out);
 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  void setValue(gin::Arguments* args);

  CbipObjectOwner* owner_;

  // The string representation of the parental rating value for the
  // respective rating scheme denoted by property scheme.
  std::string name_;
  // Unique name identifying the parental rating guidance scheme
  // to which this parental rating value refers
  std::string scheme_;
  // The parental rating value represented as an index into the set of values
  // defined as part of the ParentalRatingScheme identified through
  // property \u201cscheme\u201d.
  int value_;
  // The labels property represents a set of parental advisory flags
  // that may provide additional information about the rating.
  int labels_;
  // The region to which the parental rating value applies
  // as an alpha-2 region code as defined in ISO 3166-1.
  std::string region_;

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipObjectOipfParentalRatingClass> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipObjectOipfParentalRatingClass);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PARENTAL_RATING_CLASS_H_

