// Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/hbbtv_support/oipf_hbbtv_key_event_support.h"

#include <map>
#include <string>
#include "base/bind.h"
#include "base/logging.h"
#include "content/public/child/v8_value_converter.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "ui/events/keycodes/keyboard_codes_posix.h"
#include "v8/include/v8.h"

using namespace ui;
std::map<std::string, int> HbbtvKeyEventMap = {
  { "VK_UNDEFINED", CEA_VK_UNDEFINED },
  { "VK_CANCEL", CEA_VK_CANCEL },
  { "VK_BACK_SPACE", CEA_VK_BACK_SPACE },
  { "VK_TAB", CEA_VK_TAB },
  { "VK_CLEAR", CEA_VK_CLEAR },
  { "VK_ENTER", CEA_VK_ENTER },
  { "VK_SHIFT", CEA_VK_SHIFT },
  { "VK_CONTROL", CEA_VK_CONTROL },
  { "VK_ALT", CEA_VK_ALT },
#if defined(HBBTV_KEY_EVENT_TEST) && defined(OS_LINUX) && !defined(OS_ANDROID)
  { "VK_PAUSE", VKEY_NUMPAD1 },
#else
  { "VK_PAUSE", CEA_VK_PAUSE },
#endif
  { "VK_CAPS_LOCK", CEA_VK_CAPS_LOCK },
  { "VK_KANA", CEA_VK_KANA },
  { "VK_FINAL", CEA_VK_FINAL },
  { "VK_KANJI", CEA_VK_KANJI },
  { "VK_ESCAPE", CEA_VK_ESCAPE },
  { "VK_CONVERT", CEA_VK_CONVERT },
  { "VK_NONCONVERT", CEA_VK_NONCONVERT },
  { "VK_ACCEPT", CEA_VK_ACCEPT },
  { "VK_MODECHANGE", CEA_VK_MODECHANGE },
  { "VK_SPACE", CEA_VK_SPACE },
  { "VK_PAGE_UP", CEA_VK_PAGE_UP },
  { "VK_PAGE_DOWN", CEA_VK_PAGE_DOWN },
  { "VK_END", CEA_VK_END },
  { "VK_HOME", CEA_VK_HOME },
  { "VK_LEFT", CEA_VK_LEFT },
  { "VK_UP", CEA_VK_UP },
  { "VK_RIGHT", CEA_VK_RIGHT },
  { "VK_DOWN", CEA_VK_DOWN },
  { "VK_COMMA", CEA_VK_COMMA },
  { "VK_PERIOD", CEA_VK_PERIOD },
  { "VK_SLASH", CEA_VK_SLASH },
  { "VK_0", CEA_VK_0 },
  { "VK_1", CEA_VK_1 },
  { "VK_2", CEA_VK_2 },
  { "VK_3", CEA_VK_3 },
  { "VK_4", CEA_VK_4 },
  { "VK_5", CEA_VK_5 },
  { "VK_6", CEA_VK_6 },
  { "VK_7", CEA_VK_7 },
  { "VK_8", CEA_VK_8 },
  { "VK_9", CEA_VK_9 },
  { "VK_SEMICOLON", CEA_VK_SEMICOLON },
  { "VK_EQUALS", CEA_VK_EQUALS },
  { "VK_A", CEA_VK_A },
  { "VK_B", CEA_VK_B },
  { "VK_C", CEA_VK_C },
  { "VK_D", CEA_VK_D },
  { "VK_E", CEA_VK_E },
  { "VK_F", CEA_VK_F },
  { "VK_G", CEA_VK_G },
  { "VK_H", CEA_VK_H },
  { "VK_I", CEA_VK_I },
  { "VK_J", CEA_VK_J },
  { "VK_K", CEA_VK_K },
  { "VK_L", CEA_VK_L },
  { "VK_M", CEA_VK_M },
  { "VK_N", CEA_VK_N },
  { "VK_O", CEA_VK_O },
  { "VK_P", CEA_VK_P },
  { "VK_Q", CEA_VK_Q },
  { "VK_R", CEA_VK_R },
  { "VK_S", CEA_VK_S },
  { "VK_T", CEA_VK_T },
  { "VK_U", CEA_VK_U },
  { "VK_V", CEA_VK_V },
  { "VK_W", CEA_VK_W },
  { "VK_X", CEA_VK_X },
  { "VK_Y", CEA_VK_Y },
  { "VK_Z", CEA_VK_Z },
  { "VK_OPEN_BRACKET", CEA_VK_OPEN_BRACKET },
  { "VK_BACK_SLASH", CEA_VK_BACK_SLASH },
  { "VK_CLOSE_BRACKET", CEA_VK_CLOSE_BRACKET },
  { "VK_NUMPAD0", CEA_VK_NUMPAD0 },
  { "VK_NUMPAD1", CEA_VK_NUMPAD1 },
  { "VK_NUMPAD2", CEA_VK_NUMPAD2 },
  { "VK_NUMPAD3", CEA_VK_NUMPAD3 },
  { "VK_NUMPAD4", CEA_VK_NUMPAD4 },
  { "VK_NUMPAD5", CEA_VK_NUMPAD5 },
  { "VK_NUMPAD6", CEA_VK_NUMPAD6 },
  { "VK_NUMPAD7", CEA_VK_NUMPAD7 },
  { "VK_NUMPAD8", CEA_VK_NUMPAD8 },
  { "VK_NUMPAD9", CEA_VK_NUMPAD9 },
  { "VK_MULTIPLY", CEA_VK_MULTIPLY },
  { "VK_ADD", CEA_VK_ADD },
  { "VK_SEPARATER", CEA_VK_SEPARATER },
  { "VK_SUBTRACT", CEA_VK_SUBTRACT },
  { "VK_DECIMAL", CEA_VK_DECIMAL },
  { "VK_DIVIDE", CEA_VK_DIVIDE },
  { "VK_F1", CEA_VK_F1 },
  { "VK_F2", CEA_VK_F2 },
  { "VK_F3", CEA_VK_F3 },
  { "VK_F4", CEA_VK_F4 },
  { "VK_F5", CEA_VK_F5 },
  { "VK_F6", CEA_VK_F6 },
  { "VK_F7", CEA_VK_F7 },
  { "VK_F8", CEA_VK_F8 },
  { "VK_F9", CEA_VK_F9 },
  { "VK_F10", CEA_VK_F10 },
  { "VK_F11", CEA_VK_F11 },
  { "VK_F12", CEA_VK_F12 },
  { "VK_DELETE", CEA_VK_DELETE },
  { "VK_NUM_LOCK", CEA_VK_NUM_LOCK },
  { "VK_SCROLL_LOCK", CEA_VK_SCROLL_LOCK },
  { "VK_PRINTSCREEN", CEA_VK_PRINTSCREEN },
  { "VK_INSERT", CEA_VK_INSERT },
  { "VK_HELP", CEA_VK_HELP },
  { "VK_META", CEA_VK_META },
  { "VK_BACK_QUOTE", CEA_VK_BACK_QUOTE },
  { "VK_QUOTE", CEA_VK_QUOTE },
#if defined(HBBTV_KEY_EVENT_TEST) && defined(OS_LINUX) && !defined(OS_ANDROID)
  { "VK_RED", VKEY_F2 },
  { "VK_GREEN", VKEY_F3 },
  { "VK_YELLOW", VKEY_F4 },
  { "VK_BLUE", VKEY_F10 },
#else
  { "VK_RED", CEA_VK_RED },
  { "VK_GREEN", CEA_VK_GREEN },
  { "VK_YELLOW", CEA_VK_YELLOW },
  { "VK_BLUE", CEA_VK_BLUE },
#endif
  { "VK_GREY", CEA_VK_GREY },
  { "VK_BROWN", CEA_VK_BROWN },
  { "VK_POWER", CEA_VK_POWER },
  { "VK_DIMMER", CEA_VK_DIMMER },
  { "VK_WINK", CEA_VK_WINK },
  { "VK_EJECT_TOGGLE", CEA_VK_EJECT_TOGGLE },
#if defined(HBBTV_KEY_EVENT_TEST) && defined(OS_LINUX) && !defined(OS_ANDROID)
  { "VK_REWIND", VKEY_NUMPAD2 },
  { "VK_STOP", VKEY_NUMPAD3 },
  { "VK_PLAY", VKEY_NUMPAD4 },
  { "VK_FAST_FWD", VKEY_NUMPAD5 },
#else
  { "VK_REWIND", CEA_VK_REWIND },
  { "VK_STOP", CEA_VK_STOP },
  { "VK_PLAY", CEA_VK_PLAY },
  { "VK_FAST_FWD", CEA_VK_FAST_FWD },
#endif
  { "VK_RECORD", CEA_VK_RECORD },
  { "VK_PLAY_SPEED_UP", CEA_VK_PLAY_SPEED_UP },
  { "VK_PLAY_SPEED_DOWN", CEA_VK_PLAY_SPEED_DOWN },
  { "VK_PLAY_SPEED_RESET", CEA_VK_PLAY_SPEED_RESET },
  { "VK_RECORD_SPEED_NEXT", CEA_VK_RECORD_SPEED_NEXT },
  { "VK_GO_TO_START", CEA_VK_GO_TO_START },
  { "VK_GO_TO_END", CEA_VK_GO_TO_END },
  { "VK_TRACK_PREV", CEA_VK_PREV },
  { "VK_TRACK_NEXT", CEA_VK_NEXT },
  { "VK_RANDOM_TOGGLE", CEA_VK_RANDOM_TOGGLE },
  { "VK_CHANNEL_UP", CEA_VK_CHANNEL_UP },
  { "VK_CHANNEL_DOWN", CEA_VK_CHANNEL_DOWN },
  { "VK_STORE_FAVORITE_0", CEA_VK_STORE_FAVORITE_0 },
  { "VK_STORE_FAVORITE_1", CEA_VK_STORE_FAVORITE_1 },
  { "VK_STORE_FAVORITE_2", CEA_VK_STORE_FAVORITE_2 },
  { "VK_STORE_FAVORITE_3", CEA_VK_STORE_FAVORITE_3 },
  { "VK_RECALL_FAVORITE_0", CEA_VK_RECALL_FAVORITE_0 },
  { "VK_RECALL_FAVORITE_1", CEA_VK_RECALL_FAVORITE_1 },
  { "VK_RECALL_FAVORITE_2", CEA_VK_RECALL_FAVORITE_2 },
  { "VK_RECALL_FAVORITE_3", CEA_VK_RECALL_FAVORITE_3 },
  { "VK_CLEAR_FAVORITE_0", CEA_VK_CLEAR_FAVORITE_0 },
  { "VK_CLEAR_FAVORITE_1", CEA_VK_CLEAR_FAVORITE_1 },
  { "VK_CLEAR_FAVORITE_2", CEA_VK_CLEAR_FAVORITE_2 },
  { "VK_CLEAR_FAVORITE_3", CEA_VK_CLEAR_FAVORITE_3 },
  { "VK_SCAN_CHANNELS_TOGGLE", CEA_VK_SCAN_CHANNELS_TOGGLE },
  { "VK_PINP_TOGGLE", CEA_VK_PINP_TOGGLE },
  { "VK_SPLIT_SCREEN_TOGGLE", CEA_VK_SPLIT_SCREEN_TOGGLE },
  { "VK_DISPLAY_SWAP", CEA_VK_DISPLAY_SWAP },
  { "VK_SCREEN_MODE_NEXT", CEA_VK_SCREEN_MODE_NEXT },
  { "VK_VIDEO_MODE_NEXT", CEA_VK_VIDEO_MODE_NEXT },
  { "VK_VOLUME_UP", CEA_VK_VOLUME_UP },
  { "VK_VOLUME_DOWN", CEA_VK_VOLUME_DOWN },
  { "VK_MUTE", CEA_VK_MUTE },
  { "VK_SURROUND_MODE_NEXT", CEA_VK_SURROUND_MODE_NEXT },
  { "VK_BALANCE_RIGHT", CEA_VK_BALANCE_RIGHT },
  { "VK_BALANCE_LEFT", CEA_VK_BALANCE_LEFT },
  { "VK_FADER_FRONT", CEA_VK_FADER_FRONT },
  { "VK_FADER_REAR", CEA_VK_FADER_REAR },
  { "VK_BASS_BOOST_UP", CEA_VK_BASS_BOOST_UP },
  { "VK_BASS_BOOST_DOWN", CEA_VK_BASS_BOOST_DOWN },
  { "VK_INFO", CEA_VK_INFO },
  { "VK_GUIDE", CEA_VK_GUIDE },
  { "VK_TELETEXT", CEA_VK_TELETEXT },
  { "VK_SUBTITLE", CEA_VK_SUBTITLE },
#if defined(HBBTV_KEY_EVENT_TEST) && defined(OS_LINUX) && !defined(OS_ANDROID)
  { "VK_BACK", VKEY_NUMPAD6 },
#else
  { "VK_BACK", CEA_VK_BACK },
#endif
  { "VK_MENU", CEA_VK_MENU },
  { "VK_PLAY_PAUSE", CEA_VK_PLAY_PAUSE },
  { "VK_PORTAL", CEA_VK_PORTAL }
};

namespace content {

gin::WrapperInfo OipfHbbtvKeyEventSupport::kWrapperInfo =
    {gin::kEmbedderNativeGin};

// static
void OipfHbbtvKeyEventSupport::Install(blink::WebFrame* frame) {
  v8::Isolate* isolate = blink::mainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  if (context.IsEmpty()) {
    LOG(INFO) << "OipfHbbtvKeyEventSupport::Install Empty Context";
    return;
  }
  v8::Context::Scope context_scope(context);
  gin::Handle<OipfHbbtvKeyEventSupport> controller =
        gin::CreateHandle(isolate, new OipfHbbtvKeyEventSupport());
  if (controller.IsEmpty()) {
    LOG(INFO) << "OipfHbbtvKeyEventSupport::Install Controller Empty";
    return;
  }
  v8::Local<v8::Object> global = context->Global();
  global->Set(gin::StringToV8(isolate, "KeyEvent"), controller.ToV8());
}

v8::Local<v8::Value> OipfHbbtvKeyEventSupport::GetNamedProperty(
  v8::Isolate* isolate,
  const std::string& identifier) {
  DCHECK(isolate == isolate_);

  auto it = HbbtvKeyEventMap.find(identifier);
  if (it != HbbtvKeyEventMap.end()) {
    return v8::Integer::New(isolate, it->second);
  }

  return v8::Local<v8::Value>();
}

OipfHbbtvKeyEventSupport::OipfHbbtvKeyEventSupport()
    : gin::NamedPropertyInterceptor(blink::mainThreadIsolate(), this) {
  isolate_ = blink::mainThreadIsolate();
}

OipfHbbtvKeyEventSupport::~OipfHbbtvKeyEventSupport() {
}

gin::ObjectTemplateBuilder OipfHbbtvKeyEventSupport::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<OipfHbbtvKeyEventSupport>::GetObjectTemplateBuilder(isolate)
      .AddNamedPropertyInterceptor();
}

}  // end namespace content
