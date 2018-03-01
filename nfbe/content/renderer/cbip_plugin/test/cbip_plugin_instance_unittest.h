// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_TEST_CBIP_PLUGIN_INSTANCE_UNITTEST_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_TEST_CBIP_PLUGIN_INSTANCE_UNITTEST_H_

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/cbip_plugin/cbip_plugin_ipc.h"
#include "content/renderer/cbip_plugin/cbip_object_owner.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "content/public/test/render_view_test.h"
#include "content/test/fake_compositor_dependencies.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace content {

class CbipObjectOwnerTest : public content::CbipObjectOwner {
 public:
  CbipObjectOwnerTest();
  ~CbipObjectOwnerTest() override;
  scoped_refptr<PepperPluginInstanceImpl> instance_;
  PepperPluginInstanceImpl* instance() override;
  CbipPluginIPC* ipc() override;
  v8::Isolate* GetIsolate() override;
};

class CbipPluginInstanceUnittest : public RenderViewTest {
 public:
  CbipPluginInstanceUnittest();
  ~CbipPluginInstanceUnittest() override;
  PluginModule* module() const { return module_.get(); }
  // Provides access to the interfaces implemented by the test.
  // The default one implements PPP_INSTANCE.
  virtual const void* GetMockInterface(const char* interface_name) const;
  void SetUp() override;
  void TearDown() override;

  scoped_refptr<content::PepperPluginInstanceImpl> getCurrentInstance();
 private:
  // Note: module must be declared first since we want it to get destroyed last.
  scoped_refptr<PluginModule> module_;
  RenderFrameImpl* render_frame_impl_;
  FakeCompositorDependencies compositor_deps_;
  DISALLOW_COPY_AND_ASSIGN(CbipPluginInstanceUnittest);
};

}  // namespace content
#endif   // CONTENT_RENDERER_CBIP_PLUGIN_TEST_CBIP_PLUGIN_INSTANCE_UNITTEST_H_

