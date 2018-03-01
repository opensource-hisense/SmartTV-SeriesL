// Copyright 2015 The Chromium Embedded Framework Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2016 ACCESS LTD. All rights reserved.

#include "libcef/browser/native/browser_platform_delegate_native_aura.h"

// Include this first to avoid type conflict errors.
#include "base/tracked_objects.h"
#undef Status

#include <sys/sysinfo.h>

#include "libcef/browser/browser_host_impl.h"
#include "libcef/browser/context.h"
#include "libcef/browser/native/cef_platform_data_aura.h"
#include "libcef/browser/native/menu_runner_linux.h"
#include "libcef/browser/native/window_delegate_view.h"
#include "libcef/browser/thread_util.h"

#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/renderer_preferences.h"
#include "ui/aura/test/test_screen.h"
#include "ui/aura/env.h"
#include "ui/gfx/font_render_params.h"

namespace {

// Returns the number of seconds since system boot.
long GetSystemUptime() {
  struct sysinfo info;
  if (sysinfo(&info) == 0)
    return info.uptime;
  return 0;
}

}  // namespace

cef::PlatformDataAura* CefBrowserPlatformDelegateNativeAura::platform_ = NULL;

CefBrowserPlatformDelegateNativeAura::CefBrowserPlatformDelegateNativeAura(
    const CefWindowInfo& window_info)
: CefBrowserPlatformDelegateNative(window_info),
  host_window_created_(false),
  window_(nullptr),
  web_contents_(nullptr) {
  // Initialize the Aura platform once
  // This constructor might be called multiple times but always from
  // from the same thread so there should be no multi-threaded access here
  // hence no need for mutexes or similar guards.
  if (!platform_) {
    if (window_info_.width == 0)
      window_info_.width = 1280;
    if (window_info_.height == 0)
      window_info_.height = 720;

    gfx::Size intial_window_size(window_info_.width, window_info_.height);

    // already called in  CefBrowserMainParts::PreCreateThreads()
    // aura::TestScreen* screen = aura::TestScreen::Create(default_window_size);
    // gfx::Screen::SetScreenInstance(gfx::SCREEN_TYPE_NATIVE, screen);

    platform_ = new cef::PlatformDataAura(intial_window_size);
  }
}

CefBrowserPlatformDelegateNativeAura::~CefBrowserPlatformDelegateNativeAura() {
  CHECK(platform_);

  aura::Window* parent = platform_->host()->window();
  CHECK(parent);

  // If this was the last window release the Aura platform too.
  if (parent->children().empty()) {
    delete platform_;
    platform_ = NULL;
    // will be called in  CefBrowserMainParts::PostCreateThreads()
    // aura::Env::DeleteInstance();
  }
}


void CefBrowserPlatformDelegateNativeAura::BrowserDestroyed(
    CefBrowserHostImpl* browser) {
  CefBrowserPlatformDelegate::BrowserDestroyed(browser);

  if (host_window_created_) {
    // Release the reference added in CreateHostWindow().
    browser->Release();
  }
}

bool CefBrowserPlatformDelegateNativeAura::CreateHostWindow() {
  DCHECK(!window_);
  CHECK(platform_);

  gfx::Size default_window_size(window_info_.width, window_info_.height);

  platform_->ShowWindow();
  platform_->ResizeWindow(default_window_size);

  // Create a new window object.
  CHECK(web_contents_);
  window_ =  web_contents_->GetNativeView();
  aura::Window* parent = platform_->host()->window();
  if (!parent->Contains(window_))
    parent->AddChild(window_);

  window_->Show();

  window_info_.window = window_;

  host_window_created_ = true;

  // Add a reference that will be released in BrowserDestroyed().
  browser_->AddRef();

  // FIXME: check if we can still set the background_color if we have no widget

  // As an additional requirement on Linux, we must set the colors for the
  // render widgets in webkit.
  content::RendererPreferences* prefs =
      browser_->web_contents()->GetMutableRendererPrefs();
  prefs->focus_ring_color = SkColorSetARGB(255, 229, 151, 0);
  prefs->thumb_active_color = SkColorSetRGB(244, 244, 244);
  prefs->thumb_inactive_color = SkColorSetRGB(234, 234, 234);
  prefs->track_color = SkColorSetRGB(211, 211, 211);

  prefs->active_selection_bg_color = SkColorSetRGB(30, 144, 255);
  prefs->active_selection_fg_color = SK_ColorWHITE;
  prefs->inactive_selection_bg_color = SkColorSetRGB(200, 200, 200);
  prefs->inactive_selection_fg_color = SkColorSetRGB(50, 50, 50);

  // Set font-related attributes.
  CR_DEFINE_STATIC_LOCAL(const gfx::FontRenderParams, params,
      (gfx::GetFontRenderParams(gfx::FontRenderParamsQuery(), NULL)));
  prefs->should_antialias_text = params.antialiasing;
  prefs->use_subpixel_positioning = params.subpixel_positioning;
  prefs->hinting = params.hinting;
  prefs->use_autohinter = params.autohinter;
  prefs->use_bitmaps = params.use_bitmaps;
  prefs->subpixel_rendering = params.subpixel_rendering;

  browser_->web_contents()->GetRenderViewHost()->SyncRendererPrefs();

  return true;
}

void CefBrowserPlatformDelegateNativeAura::CloseHostWindow() {
  CHECK(platform_);
  aura::Window* parent = platform_->host()->window();
  CHECK(parent);
  if (parent->Contains(window_))
    parent->RemoveChild(window_);

  // After this point the host window is destroyed so we can notify
  // the browser so that it can die in peace
  if (browser_) {
    // Force the browser to be destroyed and release the reference added
    // in PlatformCreateWindow().
    browser_->WindowDestroyed();
  }
}

CefWindowHandle
CefBrowserPlatformDelegateNativeAura::GetHostWindowHandle() const {
  if (windowless_handler_)
    return windowless_handler_->GetParentWindowHandle();
  return window_info_.window;
}

void CefBrowserPlatformDelegateNativeAura::WebContentsCreated(
    content::WebContents* web_contents) {
  web_contents_ = web_contents;
}

views::Widget* CefBrowserPlatformDelegateNativeAura::GetWindowWidget() const {
  return NULL;
}

void CefBrowserPlatformDelegateNativeAura::SendFocusEvent(bool setFocus) {
  if (!setFocus)
    return;

  if (browser_->web_contents()) {
    // Give logical focus to the RenderWidgetHostViewAura in the views
    // hierarchy. This does not change the native keyboard focus.
    browser_->web_contents()->Focus();
  }
}

void CefBrowserPlatformDelegateNativeAura::NotifyMoveOrResizeStarted() {
  // Call the parent method to dismiss any existing popups.
  CefBrowserPlatformDelegate::NotifyMoveOrResizeStarted();

  // Send updated screen rectangle information to the renderer process so that
  // popups are displayed in the correct location.
  content::RenderWidgetHostImpl::From(
      browser_->web_contents()->GetRenderViewHost()->GetWidget())->
          SendScreenRects();
}

void CefBrowserPlatformDelegateNativeAura::SizeTo(int width, int height) {
}

gfx::Point CefBrowserPlatformDelegateNativeAura::GetScreenPoint(
    const gfx::Point& view) const {
  if (windowless_handler_)
    return windowless_handler_->GetParentScreenPoint(view);

  if (!window_)
    return view;

  // FIXME: not sure that this is actually working aura::Window::GetBoundsInScreen()
  const gfx::Rect& bounds_in_screen = window_->GetBoundsInScreen();
  return gfx::Point(bounds_in_screen.x() + view.x(),
                    bounds_in_screen.y() + view.y());
}

void CefBrowserPlatformDelegateNativeAura::ViewText(const std::string& text) {
  char buff[] = "/tmp/CEFSourceXXXXXX";
  int fd = mkstemp(buff);

  if (fd == -1)
    return;

  FILE* srcOutput = fdopen(fd, "w+");
  if (!srcOutput)
    return;

  if (fputs(text.c_str(), srcOutput) < 0) {
    fclose(srcOutput);
    return;
  }

  fclose(srcOutput);

  std::string newName(buff);
  newName.append(".txt");
  if (rename(buff, newName.c_str()) != 0)
    return;

  std::string openCommand("xdg-open ");
  openCommand += newName;

  int ret = system(openCommand.c_str());
  LOG(ERROR) << "Couldn't open a new window to display page source text(code) : " << ret;
}

void CefBrowserPlatformDelegateNativeAura::HandleKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  // Handling shortcut keys
  if (event.type == blink::WebInputEvent::RawKeyDown &&
      event.windowsKeyCode == ui::VKEY_F1) {
    // Leave the browser
    if (browser_ && browser_->destruction_state() <=
        CefBrowserHostImpl::DESTRUCTION_STATE_PENDING) {
      LOG(INFO) << "EXIT key pressed! Leaving NFBECEF...";
      CEF_POST_TASK(CEF_UIT,
          base::Bind(&CefBrowserHostImpl::CloseBrowser, browser_, true));
    }
  }
  if (event.type == blink::WebInputEvent::RawKeyDown &&
      event.windowsKeyCode == ui::VKEY_F12) {
    // Reload the start URL
    // Check if a "--url=" value was provided via the command-line.
    // If not, use the default URL.
    CefRefPtr<CefCommandLine> command_line =
        CefCommandLine::GetGlobalCommandLine();
    std::string url = command_line->GetSwitchValue("url");
    if (url.empty())
      url = "http://www.access-company.com";
    LOG(INFO) << "HOME key pressed! Reloading the start url : " << url;
    if (browser_)
      browser_->LoadURL(CefFrameHostImpl::kMainFrameId, url, content::Referrer(),
          ui::PAGE_TRANSITION_TYPED, std::string());
  }
  if (event.type == blink::WebInputEvent::RawKeyDown &&
      event.windowsKeyCode == ui::VKEY_F9) {
    LOG(INFO) << "BACK key pressed";
    if(browser_->CanGoBack()) {
      LOG(INFO) << "... going one back in the history";
      browser_->GoBack();
    } else {
      LOG(INFO) << "... CAN NOT go back in the history";
    }
  }
}

void CefBrowserPlatformDelegateNativeAura::HandleExternalProtocol(
    const GURL& url) {
}

void CefBrowserPlatformDelegateNativeAura::TranslateKeyEvent(
    content::NativeWebKeyboardEvent& result,
    const CefKeyEvent& key_event) const {
  result.timeStampSeconds = GetSystemUptime();

  result.windowsKeyCode = key_event.windows_key_code;
  result.nativeKeyCode = key_event.native_key_code;
  result.isSystemKey = key_event.is_system_key ? 1 : 0;
  switch (key_event.type) {
  case KEYEVENT_RAWKEYDOWN:
  case KEYEVENT_KEYDOWN:
    result.type = blink::WebInputEvent::RawKeyDown;
    break;
  case KEYEVENT_KEYUP:
    result.type = blink::WebInputEvent::KeyUp;
    break;
  case KEYEVENT_CHAR:
    result.type = blink::WebInputEvent::Char;
    break;
  default:
    NOTREACHED();
  }

  result.text[0] = key_event.character;
  result.unmodifiedText[0] = key_event.unmodified_character;

  result.setKeyIdentifierFromWindowsKeyCode();

  result.modifiers |= TranslateModifiers(key_event.modifiers);
}

void CefBrowserPlatformDelegateNativeAura::TranslateClickEvent(
    blink::WebMouseEvent& result,
    const CefMouseEvent& mouse_event,
    CefBrowserHost::MouseButtonType type,
    bool mouseUp, int clickCount) const {
  TranslateMouseEvent(result, mouse_event);

  switch (type) {
  case MBT_LEFT:
    result.type = mouseUp ? blink::WebInputEvent::MouseUp :
                            blink::WebInputEvent::MouseDown;
    result.button = blink::WebMouseEvent::ButtonLeft;
    break;
  case MBT_MIDDLE:
    result.type = mouseUp ? blink::WebInputEvent::MouseUp :
                            blink::WebInputEvent::MouseDown;
    result.button = blink::WebMouseEvent::ButtonMiddle;
    break;
  case MBT_RIGHT:
    result.type = mouseUp ? blink::WebInputEvent::MouseUp :
                            blink::WebInputEvent::MouseDown;
    result.button = blink::WebMouseEvent::ButtonRight;
    break;
  default:
    NOTREACHED();
  }

  result.clickCount = clickCount;
}

void CefBrowserPlatformDelegateNativeAura::TranslateMoveEvent(
    blink::WebMouseEvent& result,
    const CefMouseEvent& mouse_event,
    bool mouseLeave) const {
  TranslateMouseEvent(result, mouse_event);

  if (!mouseLeave) {
    result.type = blink::WebInputEvent::MouseMove;
    if (mouse_event.modifiers & EVENTFLAG_LEFT_MOUSE_BUTTON)
      result.button = blink::WebMouseEvent::ButtonLeft;
    else if (mouse_event.modifiers & EVENTFLAG_MIDDLE_MOUSE_BUTTON)
      result.button = blink::WebMouseEvent::ButtonMiddle;
    else if (mouse_event.modifiers & EVENTFLAG_RIGHT_MOUSE_BUTTON)
      result.button = blink::WebMouseEvent::ButtonRight;
    else
      result.button = blink::WebMouseEvent::ButtonNone;
  } else {
    result.type = blink::WebInputEvent::MouseLeave;
    result.button = blink::WebMouseEvent::ButtonNone;
  }

  result.clickCount = 0;
}

void CefBrowserPlatformDelegateNativeAura::TranslateWheelEvent(
    blink::WebMouseWheelEvent& result,
    const CefMouseEvent& mouse_event,
    int deltaX, int deltaY) const {
  result = blink::WebMouseWheelEvent();
  TranslateMouseEvent(result, mouse_event);

  result.type = blink::WebInputEvent::MouseWheel;

  static const double scrollbarPixelsPerGtkTick = 40.0;
  result.deltaX = deltaX;
  result.deltaY = deltaY;
  result.wheelTicksX = result.deltaX / scrollbarPixelsPerGtkTick;
  result.wheelTicksY = result.deltaY / scrollbarPixelsPerGtkTick;
  result.hasPreciseScrollingDeltas = true;

  // Unless the phase and momentumPhase are passed in as parameters to this
  // function, there is no way to know them
  result.phase = blink::WebMouseWheelEvent::PhaseNone;
  result.momentumPhase = blink::WebMouseWheelEvent::PhaseNone;

  if (mouse_event.modifiers & EVENTFLAG_LEFT_MOUSE_BUTTON)
    result.button = blink::WebMouseEvent::ButtonLeft;
  else if (mouse_event.modifiers & EVENTFLAG_MIDDLE_MOUSE_BUTTON)
    result.button = blink::WebMouseEvent::ButtonMiddle;
  else if (mouse_event.modifiers & EVENTFLAG_RIGHT_MOUSE_BUTTON)
    result.button = blink::WebMouseEvent::ButtonRight;
  else
    result.button = blink::WebMouseEvent::ButtonNone;
}

CefEventHandle CefBrowserPlatformDelegateNativeAura::GetEventHandle(
    const content::NativeWebKeyboardEvent& event) const {
  if (!event.os_event)
    return NULL;
  return const_cast<CefEventHandle>(event.os_event->native_event());
}

std::unique_ptr<CefMenuRunner>
CefBrowserPlatformDelegateNativeAura::CreateMenuRunner() {
  // FIXME: this should be probably null
  return make_scoped_ptr(new CefMenuRunnerLinux);
}

void CefBrowserPlatformDelegateNativeAura::TranslateMouseEvent(
    blink::WebMouseEvent& result,
    const CefMouseEvent& mouse_event) const {
  // position
  result.x = mouse_event.x;
  result.y = mouse_event.y;
  result.windowX = result.x;
  result.windowY = result.y;

  const gfx::Point& screen_pt = GetScreenPoint(gfx::Point(result.x, result.y));
  result.globalX = screen_pt.x();
  result.globalY = screen_pt.y();

  // modifiers
  result.modifiers |= TranslateModifiers(mouse_event.modifiers);

  // timestamp
  result.timeStampSeconds = GetSystemUptime();
}

