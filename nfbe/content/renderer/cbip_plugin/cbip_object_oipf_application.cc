#include "content/renderer/cbip_plugin/cbip_object_oipf_application.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/cbip/hbbtv_messages.h"
#include "content/renderer/cbip_plugin/cbip_plugin_ipc.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_application_private_data.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_application_manager.h"
#include "content/renderer/pepper/message_channel.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/pepper_try_catch.h"
#include "content/renderer/pepper/plugin_module.h"
#include "content/renderer/pepper/v8_var_converter.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "gin/public/gin_embedders.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "v8/include/v8.h"

#define X_LOG_V 0

#define X_TEST_WITHOUT_IPC 0

namespace content {

namespace {

const char kCreateApplication[] = "createApplication";
const char kDestroyApplication[] = "destroyApplication";
const char kHide[] = "hide";
const char kShow[] = "show";
const char kPrivateData[] = "privateData";
const char kMethodToString[] = "toString";

}  // namespace

// RenderFrameObserverImpl ---------------------------------------

class CbipObjectOipfApplication::RenderFrameObserverImpl
    : public base::RefCounted<CbipObjectOipfApplication::RenderFrameObserverImpl>,
      public RenderFrameObserver {
 public:
  explicit RenderFrameObserverImpl(RenderFrame* render_frame,
                                   CbipObjectOipfApplication* owner);
  void FrameDetached() override;
  bool OnMessageReceived(const IPC::Message& message) override;
 private:
  void OnHostMsgAppShownOrHidden(uint32_t oipf_app_index, bool is_shown);
  void OnHostMsgAppLoadError(uint32_t oipf_app_index);
  bool is_message_handled_;

  friend base::RefCounted<CbipObjectOipfApplication::RenderFrameObserverImpl>;
  friend RenderFrameObserver;
  ~RenderFrameObserverImpl() override;
  RenderFrame* render_frame_;
  CbipObjectOipfApplication* owner_;
  bool is_still_attached_;
};

CbipObjectOipfApplication::RenderFrameObserverImpl::RenderFrameObserverImpl(
    RenderFrame* render_frame,
    CbipObjectOipfApplication* owner)
    : RenderFrameObserver(render_frame),
      is_message_handled_(false),
      render_frame_(render_frame),
      owner_(owner),
      is_still_attached_(true) {
  // FIXME: check: make sure that |render_frame| is still attached.
  // if not, set |is_still_attached_| to false.
}

CbipObjectOipfApplication::RenderFrameObserverImpl::~RenderFrameObserverImpl() {
}

void CbipObjectOipfApplication::RenderFrameObserverImpl::FrameDetached() {
  render_frame_ = nullptr;
  is_still_attached_ = false;
  owner_->FrameDetached();
}

bool CbipObjectOipfApplication::RenderFrameObserverImpl::OnMessageReceived(
    const IPC::Message& message) {
#if 0
  if (IPC_MESSAGE_CLASS(message) == HbbtvMsgStart) {
    if (message.type() == HbbtvMsg_HOST_APP_LOAD_ERROR::ID ||
        message.type() == HbbtvMsg_HOST_APP_SHOWN_OR_HIDDEN::ID) {
      VLOG(X_LOG_V) << "ZZZZ: RenderFrameObserverImpl::OnMessageReceived:"
                    << ((message.type() ==
                         HbbtvMsg_HOST_APP_LOAD_ERROR::ID) ?
                        " HbbtvMsg_HOST_APP_LOAD_ERROR:" :
                        " HbbtvMsg_HOST_APP_SHOWN_OR_HIDDEN:")
                    << " needs handle!"
                    << " is_still_attached_=" << is_still_attached_;
        ;
    }
  }
#endif
  if (!is_still_attached_)
    return false;
  if (IPC_MESSAGE_CLASS(message) == HbbtvMsgStart) {
    if (message.type() == HbbtvMsg_HOST_APP_LOAD_ERROR::ID ||
        message.type() == HbbtvMsg_HOST_APP_SHOWN_OR_HIDDEN::ID) {
      is_message_handled_ = false;
      bool handled = true;
      IPC_BEGIN_MESSAGE_MAP(CbipObjectOipfApplication::RenderFrameObserverImpl, message)
        IPC_MESSAGE_HANDLER(HbbtvMsg_HOST_APP_LOAD_ERROR,
                            OnHostMsgAppLoadError)
        IPC_MESSAGE_HANDLER(HbbtvMsg_HOST_APP_SHOWN_OR_HIDDEN,
                            OnHostMsgAppShownOrHidden)
        IPC_MESSAGE_UNHANDLED(handled = false)
      IPC_END_MESSAGE_MAP()
      if (handled && is_message_handled_) {
        is_message_handled_ = false;
        return true;
      }
      return false;
    }
    return false;
  }
  return false;
}

void CbipObjectOipfApplication::RenderFrameObserverImpl::OnHostMsgAppLoadError(
    uint32_t oipf_app_index) {
  DCHECK(is_still_attached_ && render_frame_);
  if (owner_->app_index() != oipf_app_index) {
    // this is not the message for me.
    is_message_handled_ = false;
    return;
  }
#if 0
  VLOG(X_LOG_V) << "ZZZZ: RenderFrameObserverImpl::OnHostMsgAppLoadError:"
                << " HbbtvMsg_HOST_APP_LOAD_ERROR:"
                << " app_idx=" << oipf_app_index
                << " appmgr=" << static_cast<void*>(owner_->oipf_appmgr_)
    ;
#endif
  // FIXME: both CbipPluginObjectOipfApplicationManager::OnAppLoadError()
  // and CbipObjectOipfApplication::OnAppLoadError() need to install
  // v8::HandleScope of |isolate| and v8::Context::Scope of the window of
  // |render_frame_|.  those should be done here if performance is a concern.
  if (owner_->oipf_appmgr_) {
    owner_->oipf_appmgr_->OnAppLoadError(render_frame_, owner_,
                                         oipf_app_index);
  }
  owner_->OnAppLoadError(render_frame_);
  is_message_handled_ = true;
}

void CbipObjectOipfApplication::RenderFrameObserverImpl::OnHostMsgAppShownOrHidden(
    uint32_t oipf_app_index, bool is_shown) {
  DCHECK(is_still_attached_ && render_frame_);
  if (owner_->app_index() != oipf_app_index) {
    is_message_handled_ = false;
    return;
  }
#if 0
  VLOG(X_LOG_V) << "ZZZZ: RenderFrameObserverImpl::OnHostMsgAppShownOrHidden:"
                << " HbbtvMsg_HOST_APP_SHOWN_OR_HIDDEN:"
                << " app_idx=" << oipf_app_index
                << " is_shown=" << is_shown
                << " appmgr=" << static_cast<void*>(owner_->oipf_appmgr_)
    ;
#endif
  owner_->OnAppShownOrHidden(render_frame_, is_shown);
  is_message_handled_ = true;
}

// CbipObjectOipfApplication -------------------------------------

// only ApplicationManager should call this function.
CbipObjectOipfApplication::CbipObjectOipfApplication(
    uint32_t oipf_app_index,
    CbipPluginObjectOipfApplicationManager* oipf_appmgr)
    : gin::NamedPropertyInterceptor(oipf_appmgr->instance()->GetIsolate(), this),
      oipf_app_index_(oipf_app_index),
      oipf_appmgr_(oipf_appmgr),
      is_destroyed_by_app_(false),
      is_destroyed_by_host_(false),
      load_has_failed_(false),
      is_renderframe_still_there_(nullptr),
      weak_ptr_factory_(this) {
  /* FIXME: decide who manages Application */
  // we cannot use the plugin instance because it will be destroyed anytime.
  // we need someone of which lifecycle is the same as the window.
  v8::Isolate* isolate = oipf_appmgr_->instance()->GetIsolate();
  CbipObjectOipfApplicationPrivateData* pdata =
      new CbipObjectOipfApplicationPrivateData(this);
  privateData_.Reset(isolate, pdata->GetWrapper(isolate));

  // application manager does AddOipfApplication().

  // we need to catch messages routed to the RenderFrame that this Application
  // object belongs to.
  /* FIXME: decide who manages Application */
  // we cannot use the plugin instance because it will be destroyed anytime.
  // we need someone of which lifecycle is the same as the window.
  RenderFrameObserverImpl* impl = new RenderFrameObserverImpl(
      oipf_appmgr_->instance()->render_frame(), this);
  is_renderframe_still_there_ = impl;
}

CbipObjectOipfApplication::~CbipObjectOipfApplication() {
  {
    VLOG(X_LOG_V) << "ZZZZ: CbipObjectOipfApplication::dtor:"
                  << " this=" << static_cast<void*>(this)
                  << " oipf_app_index_=" << oipf_app_index_
                  << " oipf_appmgr_=" << static_cast<void*>(oipf_appmgr_)
                  << " is_destroyed_by_host_=" << is_destroyed_by_host_
      ;
  }

  // |v8_self| must be empty, or this wrapped object will never be destroyed.
  DCHECK(v8_self.IsEmpty());
  if (!v8_self.IsEmpty()) {
    v8_self.Reset();
  }
  // |oipf_appmgr_| must be NULL, or else, the manager forgot to unref this
  // Application object when the container window has gone.
  // failing of the below condition has some security implications because
  // this object will usually be garbage collected after the RenderFrame
  // replaces its contents.
  DCHECK(!oipf_appmgr_);
  if (oipf_appmgr_) {
    is_destroyed_by_host_ = true;  // do not send messages to the host.
    oipf_appmgr_->RemoveOipfApplication(this);
  }
}

// static
gin::WrapperInfo CbipObjectOipfApplication::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
CbipObjectOipfApplication* CbipObjectOipfApplication::FromV8Object(
    v8::Isolate* isolate,
    v8::Handle<v8::Object> v8_object) {
  CbipObjectOipfApplication* app;
  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &app)) {
    return app;
  }
  return NULL;
}

v8::Local<v8::Value> CbipObjectOipfApplication::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
#if 0
  {
    VLOG(X_LOG_V) << "ZZZZ CbipObjectOipfApplication::GetNamedProperty:"
                  << " " << identifier
      ;
  }
#endif

  if (!oipf_appmgr_ || !oipf_appmgr_->instance())
    return v8::Local<v8::Value>();

  auto find = is_property_set_.find(identifier);
  if (find != is_property_set_.end())
    return v8::Local<v8::Value>();

  if (identifier == kCreateApplication) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipObjectOipfApplication::createApplication,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kDestroyApplication) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipObjectOipfApplication::destroyApplication,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kHide) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipObjectOipfApplication::hide,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kShow) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipObjectOipfApplication::show,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kPrivateData) {
    /* FIXME: implement access control: where is that "current application" ? ? ? */
    if (load_has_failed_)
      return v8::Undefined(isolate);
    return v8::Local<v8::Object>::New(isolate, privateData_);
  } else if (identifier == kMethodToString) {
    return gin::CreateFunctionTemplate(isolate,
                                       base::Bind(&CbipObjectOipfApplication::toString,
                                                  weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  return v8::Local<v8::Value>();
}

bool CbipObjectOipfApplication::SetNamedProperty(v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (!oipf_appmgr_)
    return false;

  if (identifier == kPrivateData) {
    // This should be no effect.
    return true;
  }

  // Contents can set value to methods, so it act as normal case.
  is_property_set_.insert(identifier);
  return false;
}

std::vector<std::string> CbipObjectOipfApplication::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kCreateApplication);
  result.push_back(kDestroyApplication);
  result.push_back(kShow);
  result.push_back(kHide);
  result.push_back(kPrivateData);
  // kMethodToString isn't enumerable.

  return result;
}

PepperPluginInstanceImpl* CbipObjectOipfApplication::instance() {
  /* FIXME: decide who manages Application */
  // we cannot use the plugin instance because it will be destroyed anytime.
  // we need someone of which lifecycle is the same as the window.
  if (oipf_appmgr_)
    return oipf_appmgr_->instance();
  return nullptr;
}

CbipPluginIPC* CbipObjectOipfApplication::ipc() {
  /* FIXME: decide who manages Application */
  // we cannot use the plugin instance because it will be destroyed anytime.
  // we need someone of which lifecycle is the same as the window.
  if (oipf_appmgr_)
    return oipf_appmgr_->ipc();
  return nullptr;
}

v8::Isolate* CbipObjectOipfApplication::GetIsolate() {
  /* FIXME: decide who manages Application */
  // we cannot use the plugin instance because it will be destroyed anytime.
  // we need someone of which lifecycle is the same as the window.
  if (oipf_appmgr_ && oipf_appmgr_->instance())
    return oipf_appmgr_->instance()->GetIsolate();
  return nullptr;
}

void CbipObjectOipfApplication::ApplicationCreated() {
  v8_self.Reset(oipf_appmgr_->instance()->GetIsolate(),
      GetWrapper(oipf_appmgr_->instance()->GetIsolate()));
}

void CbipObjectOipfApplication::ApplicationDeleted() {
  /* FIXME: decide who manages Application */
  // we cannot use the plugin instance because it will be destroyed anytime.
  // we need someone of which lifecycle is the same as the window.
  if (!is_destroyed_by_host_) {
    /* FIXME: decide who manages Application */
    // we need someone of which lifecycle is the same as the window.
    // DCHECK(oipf_appmgr_);
    if (!oipf_appmgr_) {
      LOG(ERROR) << "CbipObjectOipfApplication::ApplicationDeleted: LEAK: host ref.";
    }
#if X_TEST_WITHOUT_IPC
#else
    if (oipf_appmgr_) {
      CbipPluginIPC* ipc = oipf_appmgr_->ipc();
      if (ipc) {
        int routing_id = oipf_appmgr_->instance()->render_frame()->GetRoutingID();
        ipc->ApplicationDeleted(routing_id, oipf_app_index_);
      }
    }
#endif
  }
  /* FIXME: decide who manages Application */
  // we cannot use the plugin instance because it will be destroyed anytime.
  // we need someone of which lifecycle is the same as the window.
  if (oipf_appmgr_) {
    DCHECK(oipf_appmgr_->instance());
    v8::Isolate* isolate = oipf_appmgr_->instance()->GetIsolate();
    {
      v8::HandleScope handle_scope(isolate);
      CbipObjectOipfApplicationPrivateData* pdata =
          CbipObjectOipfApplicationPrivateData::FromV8Object(isolate,
              v8::Local<v8::Object>::New(isolate, privateData_));
      DCHECK(pdata);
      pdata->OwnerApplicationDeleted();
    }
  }
  oipf_appmgr_ = 0;
  v8_self.Reset();
}

gin::ObjectTemplateBuilder CbipObjectOipfApplication::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<CbipObjectOipfApplication>::GetObjectTemplateBuilder(isolate)
      .AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor* CbipObjectOipfApplication::asGinNamedPropertyInterceptor() {
  return this;
}

void CbipObjectOipfApplication::createApplication(gin::Arguments* args) {
  {
    VLOG(X_LOG_V) << "######## OipfApplication::createApplication: ENTER";
  }

  if (load_has_failed_) {
    args->ThrowTypeError("The Application has failed to load.");
    return;
  }
  /* FIXME: decide who manages Application */
  if (!oipf_appmgr_ || !oipf_appmgr_->instance()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  CbipPluginIPC* ipc = oipf_appmgr_->ipc();
  if (!ipc) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  v8::Isolate* isolate = oipf_appmgr_->instance()->GetIsolate();
  DCHECK(isolate == args->isolate());

  v8::Handle<v8::Value> result;

  v8::Handle<v8::String> v_uri;
  v8::Handle<v8::Boolean> v_createChild;
  v8::Handle<v8::Value> v_tmp;

  if (!args->GetNext(&v_tmp)) {
    VLOG(X_LOG_V) << "######## OpifApplication::createApplication:"
                  << " error at " << __LINE__;
    args->ThrowError();
    return;
  }
  if (!v_tmp->IsString()) {
    VLOG(X_LOG_V) << "######## OpifApplication::createApplication:"
                  << " error at " << __LINE__;
    args->ThrowError();
    return;
  }
  v_uri = v8::Handle<v8::String>::Cast(v_tmp);
  v_tmp = v8::Undefined(isolate);

  if (!args->GetNext(&v_tmp)) {
    VLOG(X_LOG_V) << "######## OpifApplication::createApplication:"
                  << " error at " << __LINE__;
    args->ThrowError();
    return;
  }
  /* FIXME: what to do if |v_tmp|->IsBooleanObject() ? ? ? */
  if (!v_tmp->IsBoolean()) {
    VLOG(X_LOG_V) << "######## OpifApplication::createApplication:"
                  << " error at " << __LINE__;
    args->ThrowError();
    return;
  }
  v_createChild = v8::Handle<v8::Boolean>::Cast(v_tmp);
  v_tmp = v8::Undefined(isolate);

  /* a practical limit: 10kB */
  char uri_utf8_buf[10*1024];
  if (v_uri->Utf8Length() <= 0) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(isolate,
        "uri is too short")));
    return;
  }
  if (sizeof uri_utf8_buf <= (size_t)v_uri->Utf8Length()) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(isolate,
        "uri is too long")));
    return;
  }
  int uri_utf8_size = v_uri->WriteUtf8(uri_utf8_buf, (int)sizeof uri_utf8_buf,
      NULL, v8::String::REPLACE_INVALID_UTF8);
  if (uri_utf8_size != v_uri->Utf8Length() + 1) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(isolate,
        "something is wrong with the writer")));
    return;
  }

  bool createChild = v_createChild->IsTrue();

  {
    VLOG(X_LOG_V) << "######## OpifApplication::createApplication:"
                  << " uri=" << uri_utf8_buf
                  << " createChild=" << createChild
      ;
  }

#if X_TEST_WITHOUT_IPC
  uint32_t oipf_app_index =
      CbipPluginObjectOipfApplicationManager::new_app_index();
#else
  int routing_id = oipf_appmgr_->instance()->render_frame()->GetRoutingID();
  std::string uri_string(uri_utf8_buf);
  GURL uri(uri_string);
  // check if the given URL is not a relative one
  // e.g. for XML AIT this is frequent
  if (!uri.has_scheme()) {
      std::string base_url = oipf_appmgr_->instance()->render_frame()->GetWebFrame()->document().baseURL().string().utf8();
      GURL new_uri(base_url + uri_string);
      uri = new_uri;
      VLOG(X_LOG_V) << "######## OpifApplication::createApplication with relative path:"
                    << " uri=" << uri
                    << " base_url=" << base_url
        ;
  }

  uint32_t oipf_app_index = ipc->CreateApplication(routing_id, oipf_app_index_,
      uri, createChild);
#endif
  if (!oipf_app_index) {
    v8::Local<v8::Value> v8_null = v8::Null(isolate);
    args->Return(v8_null);
    return;
  }

  /* FIXME: decide who manages Application */
  CbipObjectOipfApplication* oipf_app =
      oipf_appmgr_->creteApp(oipf_app_index);

  args->Return(oipf_app->GetWrapper(isolate));
}

void CbipObjectOipfApplication::destroyApplication(gin::Arguments* args) {
  {
    VLOG(X_LOG_V) << "######## OipfApplication::destroyApplication: ENTER";
  }

  if (load_has_failed_) {
    args->ThrowTypeError("The Application has failed to load.");
    return;
  }
  /* FIXME: decide who manages Application */
  if (!oipf_appmgr_ || !oipf_appmgr_->instance()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  CbipPluginIPC* ipc = oipf_appmgr_->ipc();
  if (!ipc) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  v8::Isolate* isolate = oipf_appmgr_->instance()->GetIsolate();
  DCHECK(isolate == args->isolate());

  {
    VLOG(X_LOG_V) << "######## OpifApplication::destroyApplication:"
                  << " oipf_app_index_=" << oipf_app_index_
      ;
  }

#if X_TEST_WITHOUT_IPC
  (void)0;
#else
  int routing_id = oipf_appmgr_->instance()->render_frame()->GetRoutingID();
  ipc->DestroyApplication(routing_id, oipf_app_index_);
#endif

  //FIXME: rework.  we need to keep this if some event hander is registered,
  // so that, later, if the browser sends an HOST_APP_DELETED message,
  // we can generate ApplicationDestroyRequest event on this.
  {//FIXME: rework.
  // the below RemoveOipfApplication(|this|) does |v8_self|.Reset(), but
  // this should be safe, does not result in an immediate destruct of
  // this wrapped object, because at least the caller of this function has
  // a reference to the wrapper.
  oipf_appmgr_->RemoveOipfApplication(this);
  }//FIXME: remove.

  DCHECK(!oipf_appmgr_);
  // DCHECK(v8_self is reset);

  is_destroyed_by_app_ = true;

  args->Return(v8::Local<v8::Value>());  /* FIXME: void? */
}

void CbipObjectOipfApplication::show(gin::Arguments* args) {
  {
    VLOG(X_LOG_V) << "######## OipfApplication::show: ENTER";
  }

  if (load_has_failed_) {
    args->ThrowTypeError("The Application has failed to load.");
    return;
  }
  /* FIXME: decide who manages Application */
  if (!oipf_appmgr_ || !oipf_appmgr_->instance()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  CbipPluginIPC* ipc = oipf_appmgr_->ipc();
  if (!ipc) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  v8::Isolate* isolate = oipf_appmgr_->instance()->GetIsolate();
  DCHECK(isolate == args->isolate());

  {
    VLOG(X_LOG_V) << "######## OpifApplication::show:"
                  << " oipf_app_index_=" << oipf_app_index_
      ;
  }

#if X_TEST_WITHOUT_IPC
  (void)0;
#else
  int routing_id = oipf_appmgr_->instance()->render_frame()->GetRoutingID();
  ipc->Show(routing_id, oipf_app_index_);
#endif

  args->Return(v8::Local<v8::Value>());
}

void CbipObjectOipfApplication::hide(gin::Arguments* args) {
  {
    VLOG(X_LOG_V) << "######## OipfApplication::hide: ENTER";
  }

  if (load_has_failed_) {
    args->ThrowTypeError("The Application has failed to load.");
    return;
  }
  /* FIXME: decide who manages Application */
  if (!oipf_appmgr_ || !oipf_appmgr_->instance()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  CbipPluginIPC* ipc = oipf_appmgr_->ipc();
  if (!ipc) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  v8::Isolate* isolate = oipf_appmgr_->instance()->GetIsolate();
  DCHECK(isolate == args->isolate());

  {
    VLOG(X_LOG_V) << "######## OpifApplication::hide:"
                  << " oipf_app_index_=" << oipf_app_index_
      ;
  }

#if X_TEST_WITHOUT_IPC
  (void)0;
#else
  int routing_id = oipf_appmgr_->instance()->render_frame()->GetRoutingID();
  ipc->Hide(routing_id, oipf_app_index_);
#endif

  args->Return(v8::Local<v8::Value>());
}

void CbipObjectOipfApplication::toString(gin::Arguments* args) {
  v8::Handle<v8::Value> result = v8::String::NewFromUtf8(args->isolate(), "[object Application]", v8::String::kNormalString, 20);
  args->Return(result);
}

// CbipObjectOipfApplicationPrivateData::getFreeMem() calls this.
// none other should call this function.
void CbipObjectOipfApplication::getFreeMem(gin::Arguments* args)
{
  /* FIXME: decide who manages Application */
  // we cannot use the plugin instance because it will be destroyed anytime.
  // we need someone of which lifecycle is the same as the window.
  if (!oipf_appmgr_ || !oipf_appmgr_->instance()) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  CbipPluginIPC* ipc = oipf_appmgr_->ipc();
  if (!ipc) {
    args->Return(v8::Local<v8::Value>());
    return;
  }

  v8::Isolate* isolate = oipf_appmgr_->instance()->GetIsolate();
  DCHECK(isolate == args->isolate());

  if (load_has_failed_) {
    args->Return(v8::Local<v8::Value>(v8::Number::New(isolate, -1)));
    return;
  }

  v8::Local<v8::Value> result;

#if X_TEST_WITHOUT_IPC
  double value = 256 * 1024 * 1024;
#else
  int routing_id = oipf_appmgr_->instance()->render_frame()->GetRoutingID();
  double value = ipc->GetFreeMem(routing_id);
#endif

  result = v8::Number::New(isolate, value);

  args->Return(result);
}

void CbipObjectOipfApplication::FrameDetached() {
  is_renderframe_still_there_ = nullptr;
}

// OIPF DAE v1.2 (for HbbTV 1.5)
// 7.2.1.2 Methods
// function onApplicationLoadError( Application appl )
// "All properties of the Application object referred to by appl SHALL
// have the value undefined and calling any methods on that object SHALL
// fail."
// our implementation of "SHALL fail" is to throw an error.
//
// n.b. the section where onApplicationLoadError is defined seems wrong;
// it must be defined in 7.2.1.1 Properties where all event callbacks
// (onLowMemory, onApplicationLoaded, onApplicationUnloaded) are defined.
void CbipObjectOipfApplication::OnAppLoadError(RenderFrame *render_frame) {
  load_has_failed_ = true;
  // n.b. |privateData_| is not cleared now.
}

// |render_frame_| is the RenderFrame of the window.document.
// the plugin instance will go away at anytime.
void CbipObjectOipfApplication::OnAppShownOrHidden(RenderFrame *render_frame,
                                                   bool is_shown) {
  if (load_has_failed_)
    return;

  if (!oipf_appmgr_)
    return;

  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  blink::WebDocument document = frame->document();

  /* FIXME: decide who manages Application */
  // we cannot use the plugin instance because it will be destroyed anytime.
  // we need someone of which lifecycle is the same as the window.
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  DCHECK(isolate == oipf_appmgr_->instance()->GetIsolate());

  /* -- */

  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> element_obj = GetWrapper(isolate);
  v8::Local<v8::Object> window_obj = context->Global();

  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  // FIXME: check: is there any way to obtain the blink wrapper out of |document| ?

  key = v8::String::NewFromUtf8(isolate, "document", v8::String::kNormalString, 8);
  prop = window_obj->Get(key);
  if (!prop->IsObject()) {
    VLOG(X_LOG_V) << "######## CbipObjectOipfApplication " << __LINE__
                  << "'window.document' is not an object";
    return;
  }
  v8::Handle<v8::Object> document_obj = v8::Handle<v8::Object>::Cast(prop);
  key = v8::String::NewFromUtf8(isolate, "createEvent", v8::String::kNormalString, 11);
  prop = document_obj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "######## CbipObjectOipfApplication " << __LINE__
                  << "'window.document.createEvent' is not a function object";
    return;
  }
  function = v8::Handle<v8::Function>::Cast(prop);
  v8::Local<v8::Object> event_obj;
  {
    v8::Handle<v8::Value> p0 = v8::String::NewFromUtf8(
        isolate, "Event", v8::String::kNormalString, 5);
    v8::Handle<v8::Value> argv[] = { p0 };
    v8::Handle<v8::Value> result = frame->callFunctionEvenIfScriptDisabled(
        function, document_obj, 1, argv);
    if (!result->IsObject()) {
      VLOG(X_LOG_V) << "######## CbipObjectOipfApplication " << __LINE__
                    << "'window.document.createEvent' 'Event' failed";
      return;
    }
    event_obj = v8::Handle<v8::Object>::Cast(result);
  }
  key = v8::String::NewFromUtf8(
      isolate, "initEvent", v8::String::kNormalString, 9);
  prop = event_obj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "######## CbipObjectOipfApplication " << __LINE__
                  << "'Event.initEvent' is not a function object";
    return;
  }
  function = v8::Handle<v8::Function>::Cast(prop);
  {
    /* 'type' : DOMString */
    const char* cb_name;
    int cb_name_len;
    if (is_shown) {
      cb_name = "ApplicationShown";
      cb_name_len = sizeof "ApplicationShown" - 1;
    } else {
      cb_name = "ApplicationHidden";
      cb_name_len = sizeof "ApplicationHidden" - 1;
    }
    v8::Handle<v8::Value> p0 = v8::String::NewFromUtf8(
        isolate, cb_name, v8::String::kNormalString, cb_name_len);
    /* 'bubbles' : boolean */
    v8::Handle<v8::Value> p1 = v8::Boolean::New(isolate, false);
    /* 'cancelable' : boolean */
    v8::Handle<v8::Value> p2 = v8::Boolean::New(isolate, false);
    v8::Handle<v8::Value> argv[] = { p0, p1, p2 };
    v8::Handle<v8::Value> result = frame->callFunctionEvenIfScriptDisabled(
        function, event_obj, 3, argv);
    (void)result;
  }

  // FIXME: can we reuse the event object?  if we can, how?
  // e.g. the case when DOM2 handler alters the event object, is this visible to
  // DOM0 handler, or vice versa.

  /* DOM2 */
  key = v8::String::NewFromUtf8(
      isolate, "dispatchEvent", v8::String::kNormalString, 13);
  prop = element_obj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "######## CbipObjectOipfApplication " << __LINE__
                  << "'dispatchEvent' is not a function object";
    // FIXME: we need to create a DOM object....
    // return;
  } else {
    function = v8::Handle<v8::Function>::Cast(prop);
    v8::Handle<v8::Value> argv[] = { event_obj };
    v8::Handle<v8::Value> result = frame->callFunctionEvenIfScriptDisabled(
        function, element_obj, 1, argv);
    (void)result;
  }

  /* DOM0 */
  {
    const char* cb_name;
    int cb_name_len;
    if (is_shown) {
      cb_name = "onApplicationShown";
      cb_name_len = sizeof "onApplicationShown" - 1;
    } else {
      cb_name = "onApplicationHidden";
      cb_name_len = sizeof "onApplicationHidden" - 1;
    }
    key = v8::String::NewFromUtf8(isolate, cb_name, v8::String::kNormalString,
        cb_name_len);
    prop = element_obj->Get(key);
    if (!prop->IsFunction()) {
      VLOG(X_LOG_V) << "######## CbipObjectOipfApplication " << __LINE__
                    << "'" << cb_name << "'" << " is not a function object";
    }
  }
  if (prop->IsFunction()) {
    function = v8::Handle<v8::Function>::Cast(prop);
    v8::Handle<v8::Value> argv[] = { event_obj };
    v8::Handle<v8::Value> result = frame->callFunctionEvenIfScriptDisabled(
        function, element_obj, 1, argv);
    (void)result;
  }
}

}  // namespace content
