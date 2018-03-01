// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/test/cbip_plugin_instance_unittest.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "content/common/frame_messages.h"
#include "content/public/test/test_utils.h"
#if defined(HBBTV_ON)
#include "content/renderer/cbip_plugin/cbip_constants.h"
#endif
#include "content/renderer/pepper/plugin_module.h"
#include "content/renderer/render_view_impl.h"
#include "content/renderer/render_frame_impl.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/ppapi_permissions.h"
#include "v8/include/v8.h"

namespace {
const int32_t kSubframeRouteId = 20;
const int32_t kSubframeWidgetRouteId = 21;
const int32_t kFrameProxyRouteId = 22;
}  // namespace

static content::CbipPluginInstanceUnittest* g_current_unittest = nullptr;
static scoped_refptr<content::PepperPluginInstanceImpl>
    g_current_instance = nullptr;

namespace content {

CbipObjectOwnerTest::CbipObjectOwnerTest() {
}

CbipObjectOwnerTest::~CbipObjectOwnerTest() {
  instance_ = nullptr;
}

PepperPluginInstanceImpl* CbipObjectOwnerTest::instance() {
  return instance_.get();
}

CbipPluginIPC* CbipObjectOwnerTest::ipc() {
  return nullptr;
}

v8::Isolate* CbipObjectOwnerTest::GetIsolate() {
  if (g_current_instance)
    return g_current_instance->GetIsolate();
  return nullptr;
}

// PPP_Instance implementation
PP_Bool Instance_DidCreate(PP_Instance pp_instance,
                           uint32_t argc,
                           const char* argn[],
                           const char* argv[]) {
  return PP_TRUE;
}

void Instance_DidDestroy(PP_Instance instance) {
  }

void Instance_DidChangeView(PP_Instance pp_instance, PP_Resource view) {
}

void Instance_DidChangeFocus(PP_Instance pp_instance, PP_Bool has_focus) {
}

PP_Bool Instance_HandleDocumentLoad(PP_Instance pp_instance,
    PP_Resource pp_url_loader) {
  return PP_FALSE;
}

static PPP_Instance mock_instance_interface = {
    &Instance_DidCreate,
    &Instance_DidDestroy,
    &Instance_DidChangeView,
    &Instance_DidChangeFocus,
    &Instance_HandleDocumentLoad
};

int MockInitializeModule(PP_Module, PPB_GetInterface) {
  return PP_OK;
}

const void* MockGetInterface(const char* interface_name) {
  return g_current_unittest->GetMockInterface(interface_name);
}

CbipPluginInstanceUnittest::CbipPluginInstanceUnittest() {
  g_current_unittest = this;
}

CbipPluginInstanceUnittest::~CbipPluginInstanceUnittest() {
  g_current_unittest = nullptr;
}

const void* CbipPluginInstanceUnittest::GetMockInterface(
    const char* interface_name) const {
  if (strcmp(interface_name, PPP_INSTANCE_INTERFACE_1_0) == 0)
    return &mock_instance_interface;
  return NULL;
}

void CbipPluginInstanceUnittest::SetUp() {
  LOG(INFO) << "CbipPluginInstanceUnittest::SetUp";
  RenderViewTest::SetUp();
  EXPECT_TRUE(static_cast<RenderFrameImpl*>
              (view_->GetMainRenderFrame())->IsMainFrame());
  FrameMsg_NewFrame_WidgetParams widget_params;
  widget_params.routing_id = kSubframeWidgetRouteId;
  widget_params.hidden = false;
  IsolateAllSitesForTesting(base::CommandLine::ForCurrentProcess());

  LoadHTML("Hello World <iframe src='data:text/html,frame 1'></iframe>");

  RenderFrameImpl::FromWebFrame(view_->GetMainRenderFrame()->GetWebFrame())
      ->OnSwapOutWrapper(kFrameProxyRouteId, false, FrameReplicationState());
  FakeCompositorDependencies compositor_deps;
  RenderFrameImpl::CreateFrame(kSubframeRouteId, MSG_ROUTING_NONE,
                               MSG_ROUTING_NONE, kFrameProxyRouteId,
                               MSG_ROUTING_NONE, FrameReplicationState(),
                               &compositor_deps, widget_params,
                               blink::WebFrameOwnerProperties());
  render_frame_impl_ = RenderFrameImpl::FromRoutingID(kSubframeRouteId);
  EXPECT_FALSE(render_frame_impl_->IsMainFrame());
  // Initialize the mock module.
  module_ = new PluginModule("Mock plugin", "1.0",
              base::FilePath::FromUTF8Unsafe(cbip::kInternalCbipPluginFileName),
              ppapi::PpapiPermissions());
  ppapi::PpapiGlobals::Get()->ResetMainThreadMessageLoopForTesting();
  PepperPluginInfo::EntryPoints entry_points;
  entry_points.get_interface = &MockGetInterface;
  entry_points.initialize_module = &MockInitializeModule;
  ASSERT_TRUE(module_->InitAsInternalPlugin(entry_points));
  g_current_instance =
      PepperPluginInstanceImpl::Create(render_frame_impl_,
                                       module(), NULL, GURL());
}

void CbipPluginInstanceUnittest::TearDown() {
  LOG(INFO) << "CbipPluginInstanceUnittest::TearDown";
  g_current_instance = nullptr;
  module_ = nullptr;
  render_frame_impl_ = nullptr;
  RenderViewTest::TearDown();
}

scoped_refptr<content::PepperPluginInstanceImpl>
    CbipPluginInstanceUnittest::getCurrentInstance() {
  if (g_current_instance)
    return g_current_instance;
  return nullptr;
}
// Test Code End
}  // namespace content

