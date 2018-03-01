#include "content/renderer/cbip_plugin/cbip_plugin_object_js00.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/cbip/cbip_js00_messages.h"
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

using ppapi::ScopedPPVar;

namespace content {

namespace {

const char kBuiltinMethod00[] = "builtinMethod00";
const char kBuiltinMethod02[] = "builtinMethod02";
const char kBuiltinMethod03[] = "builtinMethod03";
const char kBuiltinMethod04[] = "builtinMethod04";

CbipPluginDispatcher* dispatcher() {
  return RenderThreadImpl::current()->cbip_plugin_dispatcher();
}

}  // namespace

CbipPluginObjectJs00::~CbipPluginObjectJs00() {
  if (instance_) {
    instance_->RemoveCbipPluginObject(this);
  }
  if (dispatcher())
    dispatcher()->ReleaseAppMgr(cbip_instance_id_, ipc_.get());
}

// static
gin::WrapperInfo CbipPluginObjectJs00::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
CbipPluginObjectJs00* CbipPluginObjectJs00::FromV8Object(v8::Isolate* isolate,
    v8::Handle<v8::Object> v8_object) {
  CbipPluginObjectJs00* plugin_object;

  // n.b. if the wrapper info of |v8_object| is not &CbipPluginObjectJs00::kWrapperInfo,
  // gin::ConvertFromV8() fails.

  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &plugin_object)) {
    return plugin_object;
  }
  return NULL;
}

// static
PP_Var CbipPluginObjectJs00::Create(PepperPluginInstanceImpl* instance) {
  V8VarConverter var_converter(instance->pp_instance(),
                               V8VarConverter::kAllowObjectVars);
  PepperTryCatchVar try_catch(instance, &var_converter, NULL);
  CbipPluginObjectJs00* cbip_plugin_object_js00 = new CbipPluginObjectJs00(instance);
  gin::Handle<CbipPluginObjectJs00> object =
      gin::CreateHandle(instance->GetIsolate(), cbip_plugin_object_js00);
  ScopedPPVar result = try_catch.FromV8(object.ToV8());
  DCHECK(!try_catch.HasException());
  return result.Release();
}

v8::Local<v8::Value> CbipPluginObjectJs00::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  {
    VLOG(X_LOG_V) << "ZZZZ CbipPluginObjectJs00::GetNamedProperty:"
                  << " " << identifier
      ;
  }

  if (!instance_)
    return v8::Local<v8::Value>();

  if (identifier == kBuiltinMethod00) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipPluginObjectJs00::BuiltinMethod00,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kBuiltinMethod02) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipPluginObjectJs00::BuiltinMethod02,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kBuiltinMethod03) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipPluginObjectJs00::BuiltinMethod03,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }
  if (identifier == kBuiltinMethod04) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipPluginObjectJs00::BuiltinMethod04,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  return v8::Local<v8::Value>();
}

bool CbipPluginObjectJs00::SetNamedProperty(v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (!instance_)
    return false;

  if (identifier == kBuiltinMethod00) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
       isolate, "Cannot set properties with the name builtinMethod00")));
    return true;
  }

  if (identifier == kBuiltinMethod02) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
       isolate, "Cannot set properties with the name builtinMethod02")));
    return true;
  }

  if (identifier == kBuiltinMethod03) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
       isolate, "Cannot set properties with the name builtinMethod03")));
    return true;
  }

  if (identifier == kBuiltinMethod04) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
       isolate, "Cannot set properties with the name builtinMethod04")));
    return true;
  }

  return false;
}

std::vector<std::string> CbipPluginObjectJs00::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kBuiltinMethod00);
  result.push_back(kBuiltinMethod02);
  result.push_back(kBuiltinMethod03);
  result.push_back(kBuiltinMethod04);

  return result;
}

void CbipPluginObjectJs00::InstanceDeleted() {
  instance_ = NULL;
}

CbipPluginObjectJs00::CbipPluginObjectJs00(PepperPluginInstanceImpl* instance)
    : gin::NamedPropertyInterceptor(instance->GetIsolate(), this),
      instance_(instance),
      ipc_(dispatcher()->GetAppMgr(cbip_instance_id_)),
      weak_ptr_factory_(this) {
  instance_->AddCbipPluginObject(this);
}

gin::ObjectTemplateBuilder CbipPluginObjectJs00::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<CbipPluginObjectJs00>::GetObjectTemplateBuilder(isolate)
      .AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor* CbipPluginObjectJs00::asGinNamedPropertyInterceptor() {
  return this;
}

void CbipPluginObjectJs00::BuiltinMethod00(gin::Arguments* args) {
  if (!instance_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  DCHECK(instance_->GetIsolate() == args->isolate());

  v8::Handle<v8::Value> result;

  result = v8::String::NewFromUtf8(
      instance_->GetIsolate(), "UNKNOWN", v8::String::kNormalString, 7);

  args->Return(result);
}

void CbipPluginObjectJs00::BuiltinMethod02(gin::Arguments* args) {
  if (!instance_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  DCHECK(instance_->GetIsolate() == args->isolate());

  base::string16 p0 = base::ASCIIToUTF16("da message 02");
  ipc_->PT1Async(p0);

  v8::Handle<v8::Value> result;

  result = v8::String::NewFromUtf8(
      instance_->GetIsolate(), "FIXED_VALUE_FOR_02",
      v8::String::kNormalString, 18);

  args->Return(result);
}

void CbipPluginObjectJs00::BuiltinMethod03(gin::Arguments* args) {
  if (!instance_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  DCHECK(instance_->GetIsolate() == args->isolate());

  base::string16 p0 = base::ASCIIToUTF16("da message 03");
  base::NullableString16 o0 = ipc_->PT1Sync(p0);

  {
    VLOG(X_LOG_V) << "ZZZZ: CbipPluginObjectJs00::BuiltinMethod03:"
                  << " result = " << o0;
      ;
  }

  v8::Handle<v8::Value> result;

  if (o0.is_null()) {
    result = v8::String::NewFromUtf8(
        instance_->GetIsolate(), "", v8::String::kNormalString, 0);
  } else {
    result = v8::String::NewFromTwoByte(
        instance_->GetIsolate(), o0.string().c_str());
  }

  args->Return(result);
}

void CbipPluginObjectJs00::BuiltinMethod04(gin::Arguments* args) {
  {
    VLOG(X_LOG_V) << "ZZZZ: CbipPluginObjectJs00::BuiltinMethod04:"
      ;
  }

  if (!instance_) {
    args->Return(v8::Local<v8::Value>());
    return;
  }
  DCHECK(instance_->GetIsolate() == args->isolate());

  /* FIXME: I hope this is the frame of instance_->container()
   * ->element().document().
   */
  /* FIXME: in the real world case, |args[0]| is one of the children
   * Documents, and what we need is the frame of the given document,
   * and there are out-of-process frames.  how to do?
   */
  int routing_id = instance_->render_frame()->GetRoutingID();
  base::string16 p0 = base::ASCIIToUTF16("da message 04");
  ipc_->PT1AsyncRouted(routing_id, p0);

  v8::Handle<v8::Value> result;

  result = v8::String::NewFromUtf8(
      instance_->GetIsolate(), "FIXED_VALUE_FOR_04",
      v8::String::kNormalString, 18);

  args->Return(result);
}


bool CbipPluginObjectJs00::OnAppMgrMessageReceived(const IPC::Message& msg) {
  {
    VLOG(X_LOG_V) << "ZZZZ: CbipPluginObjectJs00::OnAppMgrMessageReceived:"
      ;
  }
  IPC_BEGIN_MESSAGE_MAP(CbipPluginObjectJs00, msg)
    IPC_MESSAGE_HANDLER(CbipJs00Msg_AsyncEvent1, OnAsyncEvent1)
  IPC_END_MESSAGE_MAP()
  return false;
}

void CbipPluginObjectJs00::OnAsyncEvent1(
    const CbipJs00Msg_PT1_Params& params) {
  {
    VLOG(X_LOG_V) << "ZZZZ: CbipPluginDispatcher::OnAsyncEvent1:"
                  << " params.m0_string = " << params.m0_string
                  << " v8 current isolate = " << v8::Isolate::GetCurrent()
                  << " v8 instance isolate = " << instance_->GetIsolate()
      ;
  }

  blink::WebPluginContainer* container = instance_->container();
  if (!container) {
    return;
  }
  blink::WebElement element = container->element();
  blink::WebDocument document = element.document();
  blink::WebLocalFrame* frame = document.frame();

  /* ---- */

  DCHECK(v8::Isolate::GetCurrent() == instance_->GetIsolate());

  v8::HandleScope handle_scope(instance_->GetIsolate());
  v8::Context::Scope context_scope(instance_->GetMainWorldContext());

  v8::Handle<v8::Object> element_obj = container->v8ObjectForElement();
  v8::Local<v8::Object> window_obj = instance_->GetMainWorldContext()->Global();

  v8::Handle<v8::Value> key;
  v8::Handle<v8::Value> prop;
  v8::Handle<v8::Function> function;

  key = v8::String::NewFromUtf8(
      instance_->GetIsolate(), "document", v8::String::kNormalString, 8);
  prop = window_obj->Get(key);
  if (!prop->IsObject()) {
    VLOG(X_LOG_V) << "######## cbip_plugin_object_js00 " << __LINE__
                  << "'window.document' is not an object";
    return;
  }
  v8::Handle<v8::Object> document_obj = v8::Handle<v8::Object>::Cast(prop);
  key = v8::String::NewFromUtf8(
      instance_->GetIsolate(), "createEvent", v8::String::kNormalString, 11);
  prop = document_obj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "######## cbip_plugin_object_js00 " << __LINE__
                  << "'window.document.createEvent' is not a function object";
    return;
  }
  function = v8::Handle<v8::Function>::Cast(prop);
  v8::Local<v8::Object> event_obj;
  {
    v8::Handle<v8::Value> p0 = v8::String::NewFromUtf8(
        instance_->GetIsolate(), "Event", v8::String::kNormalString, 5);
    v8::Handle<v8::Value> argv[] = { p0 };
    v8::Handle<v8::Value> result = frame->callFunctionEvenIfScriptDisabled(function, document_obj, 1, argv);
    if (!result->IsObject()) {
      VLOG(X_LOG_V) << "######## cbip_plugin_object_js00 " << __LINE__
                    << "'window.document.createEvent' 'Event' failed";
      return;
    }
    event_obj = v8::Handle<v8::Object>::Cast(result);
  }
  key = v8::String::NewFromUtf8(
      instance_->GetIsolate(), "initEvent", v8::String::kNormalString, 9);
  prop = event_obj->Get(key);
  if (!prop->IsFunction()) {
    VLOG(X_LOG_V) << "######## cbip_plugin_object_js00 " << __LINE__
                  << "'Event.initEvent' is not a function object";
    return;
  }
  function = v8::Handle<v8::Function>::Cast(prop);
  {
    /* 'type' : DOMString */
    v8::Handle<v8::Value> p0 = v8::String::NewFromUtf8(
        instance_->GetIsolate(), "CbipEvent", v8::String::kNormalString, 9);
    /* 'bubbles' : boolean */
    v8::Handle<v8::Value> p1 = v8::Boolean::New(instance_->GetIsolate(), false);
    /* 'cancelable' : boolean */
    v8::Handle<v8::Value> p2 = v8::Boolean::New(instance_->GetIsolate(), false);
    v8::Handle<v8::Value> argv[] = { p0, p1, p2 };
    v8::Handle<v8::Value> result = frame->callFunctionEvenIfScriptDisabled(function, event_obj, 3, argv);
    (void)result;
  }
  key = v8::String::NewFromUtf8(
      instance_->GetIsolate(), "m0_string", v8::String::kNormalString, 9);
  if (params.m0_string.is_null()) {
    /* FIXME: null */
    prop = v8::String::NewFromUtf8(
        instance_->GetIsolate(), "", v8::String::kNormalString, 0);
  } else {
    prop = v8::String::NewFromTwoByte(
        instance_->GetIsolate(), params.m0_string.string().c_str());
  }
  event_obj->Set(key, prop);

  /* DOM2 */
  {
    key = v8::String::NewFromUtf8(
        instance_->GetIsolate(), "dispatchEvent", v8::String::kNormalString, 13);
    prop = element_obj->Get(key);
    if (!prop->IsFunction()) {
      VLOG(X_LOG_V) << "######## cbip_plugin_object_js00 " << __LINE__
                    << "'dispatchEvent' is not a function object";
      return;
    }
  }
  function = v8::Handle<v8::Function>::Cast(prop);
  {
    v8::Handle<v8::Value> argv[] = { event_obj };
    v8::Handle<v8::Value> result = frame->callFunctionEvenIfScriptDisabled(function, element_obj, 1, argv);
    (void)result;
  }

  /* DOM0 */
  /* FIXME: can we reuse the event object?  if we can, how? */
  {
    key = v8::String::NewFromUtf8(
        instance_->GetIsolate(), "onCbipEvent", v8::String::kNormalString, 11);
    prop = element_obj->Get(key);
    if (!prop->IsFunction()) {
      return;
    }
  }
  function = v8::Handle<v8::Function>::Cast(prop);
  {
    v8::Handle<v8::Value> argv[] = { event_obj };
    v8::Handle<v8::Value> result = frame->callFunctionEvenIfScriptDisabled(function, element_obj, 1, argv);
    (void)result;
  }
}

}  // namespace content
