// Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_HBBTV_SUPPORT_OIPF_HBBTV_KEY_EVENT_SUPPORT_H_
#define CONTENT_RENDERER_HBBTV_SUPPORT_OIPF_HBBTV_KEY_EVENT_SUPPORT_H_

#include <string>
#include "content/renderer/cbip_plugin/cbip_plugin_object.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace blink {
class WebFrame;
}

namespace content {

class OipfHbbtvKeyEventSupport
    : public gin::Wrappable<OipfHbbtvKeyEventSupport> ,
      public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;
  // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(v8::Isolate* isolate,
                                        const std::string& property) override;
  static void Install(blink::WebFrame* frame);

 private:
  v8::Isolate* isolate_;
  OipfHbbtvKeyEventSupport();
  ~OipfHbbtvKeyEventSupport() override;

  // gin::Wrappable.
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
};

}  // end namespace content
#endif  // CONTENT_RENDERER_HBBTV_SUPPORT_OIPF_HBBTV_KEY_EVENT_SUPPORT_H_
