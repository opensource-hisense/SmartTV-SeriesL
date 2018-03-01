// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/renderer/shell_content_renderer_client.h"

#include "base/command_line.h"
#include "components/web_cache/renderer/web_cache_render_process_observer.h"
#include "content/shell/renderer/shell_render_view_observer.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "v8/include/v8.h"

#if defined(ENABLE_PLUGINS)
#include "ppapi/shared_impl/ppapi_switches.h"
#if defined(HBBTV_ON)
#include "components/plugins/renderer/oipf_video_plugin.h"
#include "content/grit/content_resources.h"
#include "ui/base/resource/resource_bundle.h"
#endif
#endif

namespace content {

ShellContentRendererClient::ShellContentRendererClient() {}

ShellContentRendererClient::~ShellContentRendererClient() {
}

void ShellContentRendererClient::RenderThreadStarted() {
  web_cache_observer_.reset(new web_cache::WebCacheRenderProcessObserver());
}

void ShellContentRendererClient::RenderViewCreated(RenderView* render_view) {
  new ShellRenderViewObserver(render_view);
}

bool ShellContentRendererClient::IsPluginAllowedToUseCompositorAPI(
    const GURL& url) {
#if defined(ENABLE_PLUGINS)
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnablePepperTesting);
#else
  return false;
#endif
}

bool ShellContentRendererClient::IsPluginAllowedToUseDevChannelAPIs() {
#if defined(ENABLE_PLUGINS)
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnablePepperTesting);
#else
  return false;
#endif
}

#if defined(ENABLE_PLUGINS)
#if defined(HBBTV_ON)
bool ShellContentRendererClient::OverrideCreatePlugin(
    content::RenderFrame* render_frame,
    blink::WebLocalFrame* frame,
    const blink::WebPluginParams& params,
    blink::WebPlugin** plugin) {

  std::string orig_mime_type = params.mimeType.utf8();
  bool f_document_is_hbbtv_document = false;
  // FIXME: check the mime type of the nearest parent document,
  // and override only when necessary.
  f_document_is_hbbtv_document = true;  // always override.
  bool f_use_oipf_video_plugin = false;
  if (orig_mime_type == "application/x-oipf-video") {
    f_use_oipf_video_plugin = true;
  } else if (f_document_is_hbbtv_document) {
    if (orig_mime_type == "video/mp4" || orig_mime_type == "audio/mp4" ||
        orig_mime_type == "audio/mpeg") {
      f_use_oipf_video_plugin = true;
    }
  }
  if (f_use_oipf_video_plugin) {
    int id = IDR_OIPF_VIDEO_PLUGIN_HTML;
    base::StringPiece template_html(
        ResourceBundle::GetSharedInstance().GetRawDataResource(id));
    *plugin = (new plugins::OipfVideoPlugin(render_frame, frame, params,
                                            template_html))->plugin();
    return true;
  }

  // Allow the content module to create the plugin.
  return false;
}
#endif
#endif  //  defined(ENABLE_PLUGINS)

}  // namespace content
