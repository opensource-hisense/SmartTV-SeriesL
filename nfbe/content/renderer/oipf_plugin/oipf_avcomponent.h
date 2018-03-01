//Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_OIPF_PLUGIN_OIPF_AVCOMPONENT_H_
#define CONTENT_RENDERER_OIPF_PLUGIN_OIPF_AVCOMPONENT_H_

#include <stdint.h>
#include <string>

#include "access/peer/include/plugin/dae/video_broadcast_plugin.h"
#include "base/macros.h"
#include "v8/include/v8.h"

namespace content {

class OipfAvAudioComponent;
class OipfAvVideoComponent;
class OipfAvTextComponent;
class OipfVideoPluginObject;

class OipfAvComponent {
 public:
  virtual ~OipfAvComponent();

  typedef v8::Persistent<v8::Object,v8::CopyablePersistentTraits<v8::Object>>
      AvComponentHandle;

  // [OIPF]
  // 7.16.5.1 Media playback extensions
  // 7.16.5.1.1 Constants
  enum {
    kTypeVideo = 0,  // COMPONENT_TYPE_VIDEO
    kTypeAudio = 1,  // COMPONENT_TYPE_AUDIO
    kTypeSubtitle = 2,  // COMPONENT_TYPE_SUBTITLE
  };

  // AV control specific information.
  struct AVControlInfo {
    OipfVideoPluginObject* avcontrol;
    int avcontrol_index;
    int avcontrol_id;
  };

  virtual bool isAudio();
  virtual bool isVideo();
  virtual bool isText();
  virtual OipfAvAudioComponent* toAudio();
  virtual OipfAvVideoComponent* toVideo();
  virtual OipfAvTextComponent* toText();

 protected:
  OipfAvComponent(int componentTag, int pid, int type,
                  const std::string& encoding, bool encrypted);

  int component_tag_;
  int pid_;
  int type_;
  std::string encoding_;
  bool encrypted_;

  DISALLOW_COPY_AND_ASSIGN(OipfAvComponent);
};

}  // namespace content

#endif  // CONTENT_RENDERER_OIPF_PLUGIN_OIPF_AVCOMPONENT_H_
