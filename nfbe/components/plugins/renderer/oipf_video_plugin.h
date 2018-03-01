// Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef COMPONENTS_PLUGINS_RENDERER_OIPF_VIDEO_PLUGIN_H_
#define COMPONENTS_PLUGINS_RENDERER_OIPF_VIDEO_PLUGIN_H_

#if defined(HBBTV_ON)

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "components/plugins/renderer/plugin_placeholder.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerClient.h"
#include "third_party/WebKit/public/platform/WebSize.h"

namespace cc {
class Layer;
}  // namespace cc

namespace content {
class OipfVideoPluginObject;
}  // namespace content

namespace gin {
class Arguments;
}  // namespace gin

namespace plugins {

// this is a WebView plugin.
// plugin side scriptability is also implemented by this class.
// renderer side scriptability is implemented by OipfVideoPluginObject class.

class OipfVideoPlugin : public PluginPlaceholderBase,
                        public blink::WebMediaPlayerClient::
                                                        AcsSetWebLayerDelegate,
                        public gin::Wrappable<OipfVideoPlugin>,
                        public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  OipfVideoPlugin(content::RenderFrame* render_frame,
                  blink::WebLocalFrame* frame,
                  const blink::WebPluginParams& params,
                  base::StringPiece template_html);

  // kludge for test.
  void setInitialPlayState(double playback_rate);

  // blink::WebMediaPlayerClient::AcsSetWebLayerDelegate
  bool onSetWebLayer(blink::WebLayer*) override;

  // the below values must match those in oipf_video_plugin.html.
  enum {
    kAcsTrackTypeAudio = 0,
    kAcsTrackTypeVideo = 1,
  };

  bool CheckIfTrackIsSelected(content::OipfVideoPluginObject* self,
                              int index, int id);
  // |index| < 0: |id| is the componentType to select or unselect.
  // |index| >= 0: (|index|,|id|) is the component to select or unselect.
  // returns true if the target component to select / unselect exists.
  // e.g. returns false if the componentType is audio but the video
  //      does not have any audio track.
  bool SelectTrack(content::OipfVideoPluginObject* self, int index,
                   int id, bool select);

  double GetAspectRatio(content::OipfVideoPluginObject* self, int index,
                        int id);

  // FIXME: friend.... (anonymous)::HtmlData().
  struct PluginConfig {
    int enable_composite_kludge;
  };

  v8::Isolate* isolate_r() { return isolate_r_; }

 private:
  ~OipfVideoPlugin() override;

  // plugin side scriptability.
  void loadUrl(gin::Arguments* args);
  void onVideoSizeChanged(gin::Arguments* args);
  void handlePSC(gin::Arguments* args);
  void onTrackAdded(gin::Arguments* args);
  void onTrackRemoved(gin::Arguments* args);
  void onTrackChanged(gin::Arguments* args);

  // WebViewPlugin::Delegate
  v8::Local<v8::Value> GetV8Handle(v8::Isolate* isolate) override;
  void PluginDestroyed() override;
  v8::Local<v8::Object> GetV8ScriptableObject_1(v8::Isolate* isolate)
      override;
  void OnResize(const blink::WebSize size) override;

  // WebFrameClient
  blink::WebMediaPlayer* createMediaPlayer(
      const blink::WebURL& url,
      blink::WebMediaPlayerClient* client,
      blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
      blink::WebContentDecryptionModule* initial_cdm,
      const blink::WebString& sinkId,
      blink::WebMediaSession* wm_session) override;

  bool hasContentsLayer() const override;
  blink::WebSize contentsLayerIntrinsicSize() const override;

  // WebViewPlugin::Delegate
  bool shouldReportDetailedMessageForSourceWvD(
      const blink::WebString& source) override;
  void DetailedConsoleMessageAddedWvD(const base::string16& message,
                                      const base::string16& source,
                                      const base::string16& stack_trace,
                                      int32_t line_number,
                                      int32_t severity_level) override;

  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(v8::Isolate* isolate,
                                        const std::string& property) override;
  bool SetNamedProperty(v8::Isolate* isolate,
                        const std::string& property,
                        v8::Local<v8::Value> value) override;
  std::vector<std::string> EnumerateNamedProperties(
      v8::Isolate* isolate) override;

  // utility functions.
  v8::Local<v8::Value> GetTrackFromTlist(v8::Isolate* isolate,
                                         int tlist_idx,
                                         int acs_id);

  int visible_w_;
  int visible_h_;
  int video_w_;
  int video_h_;
  double initial_playback_rate_;

  // the blink::WebLayer of the cc::VideoLayer of the media player.
  blink::WebLayer* media_web_layer_;

  // plugin side scriptable object.
  v8::Isolate* isolate_p_;

  // renderer side scriptable object.
  v8::Isolate* isolate_r_;
  v8::Persistent<v8::Object> object_r_;

  // see the comment in source code.
  void addFullscreenStretchClass();
  base::OneShotTimer init_v8_object_timer_;
  void OnResizeDelayed();
  void OnResizeInternal();
  base::OneShotTimer onresize_timer_;

  struct PluginConfig config_;

  base::WeakPtrFactory<OipfVideoPlugin> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OipfVideoPlugin);
};

}  // namespace plugins

#endif  // HBBTV_ON

#endif  // COMPONENTS_PLUGINS_RENDERER_OIPF_VIDEO_PLUGIN_H_
