// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PROGRAMME_CLASS_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PROGRAMME_CLASS_H_

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

namespace peer {
namespace oipf {
struct VideoBroadcastProgramme;
}
}

namespace content {

class CbipObjectOwner;


class CbipObjectOipfProgrammeClass
    : public gin::Wrappable<CbipObjectOipfProgrammeClass>,
      public gin::NamedPropertyInterceptor,
      public CbipObjectOwner {
 public:
  static gin::WrapperInfo kWrapperInfo;

  explicit CbipObjectOipfProgrammeClass(
    CbipObjectOwner* owner,
    const peer::oipf::VideoBroadcastProgramme& programme);

  ~CbipObjectOipfProgrammeClass() override;

  static CbipObjectOipfProgrammeClass* FromV8Object(
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

  void GetSIDescriptors(gin::Arguments* args);

  gin::NamedPropertyInterceptor* asGinNamedPropertyInterceptor();

  PepperPluginInstanceImpl* instance() override;

  CbipPluginIPC* ipc() override;

  v8::Isolate* GetIsolate() override;

 private:
  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  void setValue(gin::Arguments* args);

  CbipObjectOwner* owner_;

  // The short name of the programme
  std::string name_;
  // The unique identifier of the programme or series
  std::string programme_id_;
  // The type of identification used to reference the programme,
  // as indicated by one of the ID_* constants defined above
  int programme_id_type_;
  // The description of the programme, e.g. an episode synopsis.
  // If no description is available, this property will be undefined
  std::string description_;
  // The long description of the programme.
  // If no description is available, this property will be undefined.
  std::string long_description_;
  // The start time of the programme, measured in seconds
  // since midnight (GMT) on 1/1/1970.
  uint64_t start_time_;
  // The duration of the programme (in seconds).
  uint64_t duration_;
  // The identifier of the channel from which the broadcasted content
  // is to be recorded. Specifies either a ccid or ipBroadcastID
  std::string channel_id_;
  // TODO(user): implement ParentalRatingCollection
  v8::Persistent<v8::Object> parental_rating_collection_;

  // This is used to ensure pending tasks will not fire after this object is
  // destroyed.
  base::WeakPtrFactory<CbipObjectOipfProgrammeClass> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CbipObjectOipfProgrammeClass);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OIPF_PROGRAMME_CLASS_H_

