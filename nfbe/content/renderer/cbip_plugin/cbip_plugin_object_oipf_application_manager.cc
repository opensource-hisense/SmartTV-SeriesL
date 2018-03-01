#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_application_manager.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/cbip/hbbtv_messages.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_application.h"
#include "content/renderer/cbip_plugin/cbip_plugin_dispatcher.h"
#include "content/renderer/cbip_plugin/cbip_plugin_ipc.h"
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
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"
#include "v8/include/v8.h"

#define X_LOG_V 0

#define X_TEST_WITHOUT_IPC 0

using ppapi::ScopedPPVar;

namespace content {

namespace {

const char kGetOwnerApplication[] = "getOwnerApplication";
const char kToString[] = "toString";

CbipPluginDispatcher* dispatcher() {
  return RenderThreadImpl::current()->cbip_plugin_dispatcher();
}

// FIXME: there should be a flag to enable / disable logging.
// FIXME: there should be a flag to choose VLOG level.
// this is a potentially dangerous operation, so call this with care.
static void VLogThrownErrorMessage(v8::TryCatch& try_catch,
                                   v8::Isolate* isolate,
                                   const std::string& log_tag,
                                   int lineno) {
  v8::Local<v8::String> ss;
  v8::MaybeLocal<v8::String> ms = try_catch.Exception()->ToString(isolate);
  if (ms.ToLocal(&ss)) {
    char buf[256];
    int r = ss->WriteUtf8(buf, 255, NULL,
                          (v8::String::NO_NULL_TERMINATION|
                           v8::String::REPLACE_INVALID_UTF8));
    DCHECK(r <= 255);
    buf[r] = '\0';
    VLOG(0) << log_tag << " " << lineno << " " << buf;
  }
}

static uint32_t g_app_index = 1;

}  // namespace

// only for testing until IPC is implemented.
// static
uint32_t CbipPluginObjectOipfApplicationManager::new_app_index() {
  return g_app_index++;
}

CbipPluginObjectOipfApplicationManager::~CbipPluginObjectOipfApplicationManager() {
  if (instance_) {
    instance_->RemoveCbipPluginObject(this);
  }
  if (dispatcher())
    dispatcher()->ReleaseAppMgr(cbip_instance_id_, ipc_.get());
}

// static
gin::WrapperInfo CbipPluginObjectOipfApplicationManager::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
CbipPluginObjectOipfApplicationManager* CbipPluginObjectOipfApplicationManager::FromV8Object(
    v8::Isolate* isolate, v8::Handle<v8::Object> v8_object) {
  CbipPluginObjectOipfApplicationManager* plugin_object;

  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &plugin_object)) {
    return plugin_object;
  }
  return NULL;
}

// static
PP_Var CbipPluginObjectOipfApplicationManager::Create(PepperPluginInstanceImpl* instance) {
  V8VarConverter var_converter(instance->pp_instance(),
                               V8VarConverter::kAllowObjectVars);
  PepperTryCatchVar try_catch(instance, &var_converter, NULL);
  CbipPluginObjectOipfApplicationManager* plugin_object =
      new CbipPluginObjectOipfApplicationManager(instance);
  gin::Handle<CbipPluginObjectOipfApplicationManager> object =
      gin::CreateHandle(instance->GetIsolate(), plugin_object);
  ScopedPPVar result = try_catch.FromV8(object.ToV8());
  DCHECK(!try_catch.HasException());
  return result.Release();
}

v8::Local<v8::Value> CbipPluginObjectOipfApplicationManager::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
#if 0
  {
    VLOG(X_LOG_V) << "ZZZZ CbipPluginObjectOipfApplicationManager::GetNamedProperty:"
                  << " this=" << static_cast<void*>(this)
                  << " isolate=" << isolate
                  << " " << identifier
      ;
  }
#endif

  if (!instance_)
    return v8::Local<v8::Value>();

  if (identifier == kGetOwnerApplication) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipPluginObjectOipfApplicationManager::getOwnerApplication,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kToString) {
    return gin::CreateFunctionTemplate(
        isolate,
        base::Bind(&CbipPluginObjectOipfApplicationManager::toString,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  return v8::Local<v8::Value>();
}

bool CbipPluginObjectOipfApplicationManager::SetNamedProperty(v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
#if 0
  {
    VLOG(X_LOG_V) << "ZZZZ CbipPluginObjectOipfApplicationManager::SetNamedProperty:"
                  << " this=" << static_cast<void*>(this)
                  << " isolate=" << isolate
                  << " " << identifier
      ;
  }
#endif

  if (!instance_)
    return false;

  if (identifier == kGetOwnerApplication) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
       isolate, "Cannot set properties with the name getOwnerApplication")));
    return true;
  }

  return false;
}

std::vector<std::string> CbipPluginObjectOipfApplicationManager::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kGetOwnerApplication);
  // kToString isn't enumerable.

  return result;
}

void CbipPluginObjectOipfApplicationManager::RemoveOipfApplication(
    CbipObjectOipfApplication* app_object) {
  uint32_t app_index = app_object->app_index();
  // FIXME: this assumes that this plugin does not have many Documents.
  for (std::map<void*,uint32_t>::iterator it = owner_applications_.begin();
       it != owner_applications_.end();
       ++it) {
    if (it->second == app_index) {
      owner_applications_.erase(it);
      break;
    }
  }
#if X_MAP_TO_HAVE_V8_PERSISTENT
#error
#else
  {
    std::map<uint32_t,CbipObjectOipfApplication*>::iterator it =
        live_applications_.find(app_index);
    if (it != live_applications_.end()) {
      DCHECK(it->second == app_object);
      app_object->ApplicationDeleted();
      live_applications_.erase(it);
    }
  }
#endif
}

void CbipPluginObjectOipfApplicationManager::InstanceDeleted() {
  owner_applications_.clear();

  // Swap out the set so we can delete from it (the objects will try to
  // unregister themselves inside the delete call.)
  // (Well the objects do not do this in the current implementations,
  // but this safe, and this is what PepperPluginInstanceImpl does
  // on its PluginObject.
#if X_MAP_TO_HAVE_V8_PERSISTENT
  std::map<uint32_t,v8::Persistent<v8::Object>> live_applications_copy;
  live_applications_.swap(live_applications_copy);
  for (std::map<uint32_t,v8::Persistent<v8::Object>>::iterator it =
           live_applications_copy.begin();
       it != live_applications_copy.end();
       ++it) {
    CbipObjectOipfApplication* o = CbipObjectOipfApplication::FromV8Object(
        instance_->GetIsolate(),
        v8::Local<v8::Object>::New(instance_->GetIsolate(), it->second));
    DCHECK(o);
    if (o) {
      o->ApplicationDeleted();
    }
  }
#else
  std::map<uint32_t,CbipObjectOipfApplication*> live_applications_copy;
  live_applications_.swap(live_applications_copy);
  for (std::map<uint32_t,CbipObjectOipfApplication*>::iterator it =
           live_applications_copy.begin();
       it != live_applications_copy.end();
       ++it) {
    CbipObjectOipfApplication* o = it->second;
    DCHECK(o);
    if (o) {
      o->ApplicationDeleted();
    }
  }
#endif

  instance_ = NULL;
}

CbipPluginObjectOipfApplicationManager::CbipPluginObjectOipfApplicationManager(
    PepperPluginInstanceImpl* instance)
    : gin::NamedPropertyInterceptor(instance->GetIsolate(), this),
      instance_(instance),
      ipc_(dispatcher()->GetAppMgr(cbip_instance_id_)),
      weak_ptr_factory_(this) {
  instance_->AddCbipPluginObject(this);
}

gin::ObjectTemplateBuilder CbipPluginObjectOipfApplicationManager::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<CbipPluginObjectOipfApplicationManager>::GetObjectTemplateBuilder(isolate)
      .AddNamedPropertyInterceptor();
}

void CbipPluginObjectOipfApplicationManager::toString(gin::Arguments* args) {
  std::string class_name("[object ApplicationManager]");
  v8::Handle<v8::Value> result =
      v8::String::NewFromUtf8(args->isolate(), class_name.c_str(),
                              v8::String::kNormalString,
                              class_name.length());
  args->Return(result);
}

gin::NamedPropertyInterceptor* CbipPluginObjectOipfApplicationManager::asGinNamedPropertyInterceptor() {
  return this;
}

void CbipPluginObjectOipfApplicationManager::getOwnerApplication(gin::Arguments* args) {
  if (!instance_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  DCHECK(instance_->GetIsolate() == args->isolate());

  blink::WebPluginContainer* container = instance_->container();
  if (!container) {
    args->ThrowTypeError("container is NULL.");
    return;
  }

  blink::WebElement element = container->element();
  blink::WebDocument document = element.document();

  v8::Handle<v8::Object> v_document;
  v8::Handle<v8::Value> result;
  v8::Handle<v8::Value> v_tmp;

  if (!args->GetNext(&v_tmp)) {
    VLOG(X_LOG_V) << "######## OipfApplicationManager::getOwnerApplication:"
                  << " error at " << __LINE__;
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(instance_->GetIsolate())));
    return;
  }
  if (!v_tmp->IsObject()) {
    VLOG(X_LOG_V) << "######## OipfApplicationManager::getOwnerApplication:"
                  << " error at " << __LINE__;
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(instance_->GetIsolate())));
    return;
  }
  v_document = v8::Handle<v8::Object>::Cast(v_tmp);

  if (args->GetNext(&v_tmp)) {
    // Too many arguments
    VLOG(X_LOG_V) << "######## OipfApplicationManager::getOwnerApplication:"
                  << " error at " << __LINE__;
    args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(instance_->GetIsolate())));
    return;
  }

  // FIXME: if we are in blink, what we will do is,
  // blink::Document* doc = V8Document::toImplWithTypeCheck(isolate, v_document),
  // and check if blink::WebDocument has private blink::Document.
  // but we are not in blink.  how to do?
  {
    // check against window.document;
    v8::Local<v8::Object> window_obj = instance_->GetMainWorldContext()->Global();
    v8::Handle<v8::Value> key;
    v8::Handle<v8::Value> prop;
    key = v8::String::NewFromUtf8(
        instance_->GetIsolate(), "document", v8::String::kNormalString, 8);
    prop = window_obj->Get(key);
    if (!prop->IsObject()) {
      VLOG(X_LOG_V) << "######## OipfApplicationManager::getOwnerApplication:"
                    << " error at " << __LINE__;
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(instance_->GetIsolate())));
      return;
    }
    v8::Handle<v8::Object> document_obj = v8::Handle<v8::Object>::Cast(prop);
    if (document_obj != v_document) {
      VLOG(X_LOG_V) << "######## OipfApplicationManager::getOwnerApplication:"
                    << " error at " << __LINE__;
      args->Return(static_cast<v8::Local<v8::Value>>(v8::Null(instance_->GetIsolate())));
      return;
    }
  }

  void* p_document = 0;  /* FIXME: currently window.document maps to 0. */

  CbipObjectOipfApplication* oipf_app = 0;

  {
    std::map<void*,uint32_t>::iterator it =
        owner_applications_.find(p_document);
    if (it != owner_applications_.end()) {
      uint32_t oipf_app_index = it->second;
      DCHECK(live_applications_.find(oipf_app_index) != live_applications_.end());
#if X_MAP_TO_HAVE_V8_PERSISTENT
      {
        v8::Persistent<v8::Object>& ph = live_applications_.find(oipf_app_index)->second;
        v8::Local<v8::Object> lh = v8::Local<v8::Object>::New(instance_->GetIsolate(), ph);
        oipf_app = CbipObjectOipfApplication::FromV8Object(instance_->GetIsolate(), lh);
      }
      DCHECK(oipf_app);
#else
      oipf_app = live_applications_.find(oipf_app_index)->second;
#endif
      DCHECK(oipf_app->app_index() == oipf_app_index);
    }
  }
  if (!oipf_app) {
#if X_TEST_WITHOUT_IPC
    uint32_t oipf_app_index = new_app_index();
#else
    /* FIXME: this is only for the case when |args[0]| is window.document. */
    int routing_id = instance_->render_frame()->GetRoutingID();
    uint32_t oipf_app_index = ipc_->GetOwnerApplication0(routing_id);
    {
      VLOG(X_LOG_V) << "ZZZZ getOwnerApplication: app_index=" << oipf_app_index;
    }
#endif
    if (!oipf_app_index) {
      v8::Local<v8::Value> v8_null = v8::Null(args->isolate());
      args->Return(v8_null);
      return;
    }
    DCHECK(owner_applications_.find(p_document) == owner_applications_.end());
    DCHECK(live_applications_.find(oipf_app_index) == live_applications_.end());
    oipf_app = new CbipObjectOipfApplication(oipf_app_index, this);
    owner_applications_[p_document] = oipf_app_index;
#if X_MAP_TO_HAVE_V8_PERSISTENT
    {
      v8::Local<v8::Object> lh = oipf_app->GetWrapper(instance_->GetIsolate());
      live_applications_[oipf_app_index].Reset(instance_->GetIsolate(), lh);
    }
#else
    oipf_app->ApplicationCreated();
    live_applications_[oipf_app_index] = oipf_app;
#endif
  }

  args->Return(oipf_app->GetWrapper(instance_->GetIsolate()));
}

CbipObjectOipfApplication* CbipPluginObjectOipfApplicationManager::creteApp(
    uint32_t oipf_app_index) {
  CbipObjectOipfApplication* oipf_app = 0;

#if X_MAP_TO_HAVE_V8_PERSISTENT
  std::map<uint32_t,v8::Persistent<v8::Object>>::iterator it =
      live_applications_.find(oipf_app_index);
#else
  std::map<uint32_t,CbipObjectOipfApplication*>::iterator it =
      live_applications_.find(oipf_app_index);
#endif
  if (it != live_applications_.end()) {
#if X_MAP_TO_HAVE_V8_PERSISTENT
    {
      v8::Persistent<v8::Object>& ph = it->second;
      v8::Local<v8::Object> lh = v8::Local<v8::Object>::New(instance_->GetIsolate(), ph);
      oipf_app = CbipObjectOipfApplication::FromV8Object(instance_->GetIsolate(), lh);
    }
    DCHECK(oipf_app);
#else
    oipf_app = it->second;
#endif
    DCHECK(oipf_app->app_index() == oipf_app_index);
    return oipf_app;
  }

  oipf_app = new CbipObjectOipfApplication(oipf_app_index, this);
#if X_MAP_TO_HAVE_V8_PERSISTENT
  {
    v8::Local<v8::Object> lh = oipf_app->GetWrapper(instance_->GetIsolate());
    live_applications_[oipf_app_index].Reset(instance_->GetIsolate(), lh);
  }
#else
  oipf_app->ApplicationCreated();
  live_applications_[oipf_app_index] = oipf_app;
#endif
  return oipf_app;
}

// |render_frame|: the RenderFrame of the window.document.
//   the caller has already made sure that this is not NULL.
// |oipf_app|: the application of which load failed.
// |oipf_app_index|: the application_index of |oipf_app|.
void CbipPluginObjectOipfApplicationManager::OnAppLoadError(
    RenderFrame* render_frame,
    CbipObjectOipfApplication* oipf_app,
    uint32_t oipf_app_index) {

  VLOG(X_LOG_V) << "CbipPluginObjectOipfApplicationManager::OnAppLoadError:"
      ;

  DCHECK(render_frame);
  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  blink::WebDocument document = frame->document();

  /* FIXME: decide who manages Application */
  // we cannot use the plugin instance because it will be destroyed anytime.
  // we need someone of which lifecycle is the same as the window.
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  DCHECK(isolate == instance_->GetIsolate());

  /* -- */

  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  v8::Context::Scope context_scope(context);

  v8::Handle<v8::Object> element_obj =
      instance_->container()->v8ObjectForElement();
  if (element_obj->IsNull() || !element_obj->IsObject())
    return;

  // check if |oipf_app| is still valid.
#if X_MAP_TO_HAVE_V8_PERSISTENT
#error impl
#else
  {
    std::map<uint32_t,CbipObjectOipfApplication*>::iterator it =
        live_applications_.find(oipf_app_index);
    if (it == live_applications_.end()) {
      // this happens when browser detecting load error and renderer
      // destroying |oipf_app_index| happen at the same time.
      VLOG(X_LOG_V) << "ZZZZ: OnAppLoadError: error: " << __LINE__;
      return;
    }
    DCHECK(it->second == oipf_app);
    if (it->second != oipf_app) {
      // something is seriously wrong.
      VLOG(X_LOG_V) << "ZZZZ: OnAppLoadError: error: " << __LINE__;
      return;
    }
  }
  v8::Local<v8::Object> app_obj = oipf_app->GetWrapper(isolate);
#endif

  v8::Local<v8::Object> window_obj = context->Global();

  v8::MaybeLocal<v8::String> mbl;
  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  // FIXME: check: is there any way to obtain the v8::Object of |document|?

  mbl = v8::String::NewFromUtf8(isolate, "document", v8::String::kNormalString,
                                8);
  if (!mbl.ToLocal(&key))
    return;
  prop = window_obj->Get(key);
  if (!prop->IsObject()) {
    VLOG(X_LOG_V) << "######## OipfApplicationManager: " << __LINE__
                  << "'window.document' is not an object";
    return;
  }
  v8::Handle<v8::Object> document_obj = v8::Handle<v8::Object>::Cast(prop);

  mbl = v8::String::NewFromUtf8(isolate, "createEvent",
                                v8::String::kNormalString, 11);
  if (!mbl.ToLocal(&key))
    return;
  prop = document_obj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "######## OipfApplicationManager: " << __LINE__
                  << " 'window.document.createEvent' is not a function object";
    return;
  }
  function = v8::Handle<v8::Function>::Cast(prop);

  v8::Local<v8::Object> event_obj;
  {
    v8::MaybeLocal<v8::String> p0_mbl = v8::String::NewFromUtf8(
        isolate, "Event", v8::String::kNormalString, 5);
    v8::Handle<v8::Value> p0;
    if (!p0_mbl.ToLocal(&p0))
      return;
    v8::Handle<v8::Value> argv[] = { p0 };
    v8::TryCatch try_catch(isolate);
    v8::Handle<v8::Value> result = frame->callFunctionEvenIfScriptDisabled(
        function, document_obj, 1, argv);
    if (try_catch.HasCaught()) {
      VLogThrownErrorMessage(
          try_catch, isolate,
          std::string("######## OipfApplicationManager: error:"), __LINE__);
      return;
    }
    if (result->IsNull() || !result->IsObject()) {
      VLOG(X_LOG_V) << "######## OipfApplicationManager: " << __LINE__
                    << " 'window.document.createEvent' 'Event' failed";
      return;
    }
    event_obj = v8::Handle<v8::Object>::Cast(result);
  }

  mbl = v8::String::NewFromUtf8(isolate, "initEvent",
                                v8::String::kNormalString, 9);
  if (!mbl.ToLocal(&key))
    return;
  prop = event_obj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "######## OipfApplicationManager: " << __LINE__
                  << " 'Event.initEvent' is not a function object";
    return;
  }
  function = v8::Handle<v8::Function>::Cast(prop);
  {
    /* 'type' : DOMString */
    v8::MaybeLocal<v8::String> p0_mbl = v8::String::NewFromUtf8(
        isolate, "ApplicationLoadError", v8::String::kNormalString, 20);
    v8::Handle<v8::Value> p0;
    if (!p0_mbl.ToLocal(&p0))
      return;
    /* 'bubbles' : boolean */
    v8::Handle<v8::Value> p1 = v8::Boolean::New(isolate, false);
    /* 'cancelable' : boolean */
    v8::Handle<v8::Value> p2 = v8::Boolean::New(isolate, false);
    v8::Handle<v8::Value> argv[] = { p0, p1, p2 };
    v8::TryCatch try_catch(isolate);
    v8::Handle<v8::Value> result = frame->callFunctionEvenIfScriptDisabled(
        function, event_obj, 3, argv);
    if (try_catch.HasCaught()) {
      VLogThrownErrorMessage(
          try_catch, isolate,
          std::string("######## OipfApplicationManager: error:"), __LINE__);
      return;
    }
    (void)result;
  }

  mbl = v8::String::NewFromUtf8(isolate, "appl",
                                v8::String::kNormalString, 4);
  if (!mbl.ToLocal(&key))
    return;
  event_obj->Set(key, app_obj);

  // FIXME: check: the order of calling handlers.
  // currently DOM2 style handlers are called first, then DOM0 style handler.
  // i.e. the changes that DOM0 handler does on |app_obj| are invisible
  // to DOM2 style handlers.
  do {
    mbl = v8::String::NewFromUtf8(isolate, "dispatchEvent",
                                  v8::String::kNormalString, 13);
    if (!mbl.ToLocal(&key))
      break;
    prop = element_obj->Get(key);
    if (!prop->IsFunction()) {
      VLOG(X_LOG_V) << "######## OipfApplicationManager: " << __LINE__
                    << __LINE__
                    << " 'dispatchEvent' is not a function object";
      break;
    }
    function = v8::Handle<v8::Function>::Cast(prop);
    v8::Handle<v8::Value> argv[] = { event_obj };
    // n.b. this is _just_ in case when "dispatchEvent" is tampered.
    v8::TryCatch try_catch(isolate);  // automatic error remover.
    v8::Handle<v8::Value> result = frame->callFunctionEvenIfScriptDisabled(
        function, element_obj, 1, argv);
#if 1  // FIXME: flag to disable log.  this error is not fatal.
    if (try_catch.HasCaught()) {
      VLogThrownErrorMessage(
          try_catch, isolate,
          std::string("######## OipfApplicationManager: error:"), __LINE__);
    }
#endif
    (void)result;
  } while (0);

  /* DOM0 */
  do {
    mbl = v8::String::NewFromUtf8(isolate, "onApplicationLoadError",
                                  v8::String::kNormalString, 22);
    if (!mbl.ToLocal(&key))
      break;
    prop = element_obj->Get(key);
    if (!prop->IsFunction()) {
      VLOG(X_LOG_V) << "######## OipfApplicationManager: " << __LINE__
                    << " 'onApplicationLoadError' is not a function object";
      break;
    }
    function = v8::Handle<v8::Function>::Cast(prop);
    v8::Handle<v8::Value> argv[] = { app_obj };
    v8::TryCatch try_catch(isolate);  // automatic error remover.
    v8::Handle<v8::Value> result = frame->callFunctionEvenIfScriptDisabled(
        function, element_obj, 1, argv);
#if 1  // FIXME: flag to disable log.  this error is not fatal.
    if (try_catch.HasCaught()) {
      VLogThrownErrorMessage(
          try_catch, isolate,
          std::string("######## OipfApplicationManager: error:"), __LINE__);
    }
#endif
    (void)result;
  } while (0);
}

}  // namespace content
