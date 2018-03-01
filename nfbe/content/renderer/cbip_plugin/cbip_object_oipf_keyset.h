// Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_KEYSET_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_KEYSET_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"

namespace IPC {
class Message;
}

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

class CbipObjectOipfApplicationPrivateData;

class CbipObjectOipfKeyset
    : public gin::Wrappable<CbipObjectOipfKeyset>,
      public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  // nobody except CbipPluginObjectOipfApplicationPrivateData should call.
  // FIXME: friend?
  explicit CbipObjectOipfKeyset(
      CbipObjectOipfApplicationPrivateData* owner);

  ~CbipObjectOipfKeyset() override;

  static CbipObjectOipfKeyset* FromV8Object(
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

  void OwnerDeleted();

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor();

 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  void setValue(gin::Arguments* args);

  CbipObjectOipfApplicationPrivateData* owner_;

  // readonly Integer value
  int value_;
  // readonly Integer otherKeys[]
  // currently unsupported.
  // readonly Integer maximumValue
  // currently this is a fixed set.
  // readonly Integer maximumOtherkeys[]
  // currently this is a fixed set.

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipObjectOipfKeyset> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipObjectOipfKeyset);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_KEYSET_H_
