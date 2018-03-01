
// Copyright (c) 2016 ACCESS CO. LTD. All rights reserved.

// cef_platform_data_aura.cc
// Based on content shell's shell_platform_data_aura.cc

#include "libcef/browser/native/cef_platform_data_aura.h"

#include "ui/aura/window.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/test/test_focus_client.h"
#include "ui/aura/test/test_window_tree_client.h"
#include "ui/aura/env.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/ime/input_method_delegate.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/input_method_factory.h"
#include "ui/gfx/screen.h"
#include "ui/wm/core/default_activation_client.h"


namespace cef {

class FillLayout : public aura::LayoutManager {
 public:
  explicit FillLayout(aura::Window* root)
      : root_(root) {
  }

  virtual ~FillLayout() {}

 private:
  // aura::LayoutManager:
  virtual void OnWindowResized() override {
  }

  virtual void OnWindowAddedToLayout(aura::Window* child) override {
    child->SetBounds(root_->bounds());
  }

  virtual void OnWillRemoveWindowFromLayout(aura::Window* child) override {
  }

  virtual void OnWindowRemovedFromLayout(aura::Window* child) override {
  }

  virtual void OnChildWindowVisibilityChanged(aura::Window* child,
                                              bool visible) override {
  }

  virtual void SetChildBounds(aura::Window* child,
                              const gfx::Rect& requested_bounds) override {
    SetChildBoundsDirect(child, requested_bounds);
  }

  aura::Window* root_;

  DISALLOW_COPY_AND_ASSIGN(FillLayout);
};

PlatformDataAura::PlatformDataAura(const gfx::Size& initial_size) {
  CHECK(aura::Env::GetInstance());
  host_.reset(aura::WindowTreeHost::Create(gfx::Rect(initial_size)));
  host_->InitHost();
  host_->window()->SetLayoutManager(new FillLayout(host_->window()));

  focus_client_.reset(new aura::test::TestFocusClient());
  aura::client::SetFocusClient(host_->window(), focus_client_.get());

  new wm::DefaultActivationClient(host_->window());
  capture_client_.reset(
      new aura::client::DefaultCaptureClient(host_->window()));
  window_tree_client_.reset(
      new aura::test::TestWindowTreeClient(host_->window()));
}

void PlatformDataAura::ShowWindow() {
    host_->Show();
}

void PlatformDataAura::ResizeWindow(const gfx::Size& size) {
    host_->SetBounds(gfx::Rect(size));
}

}//namespace cef
