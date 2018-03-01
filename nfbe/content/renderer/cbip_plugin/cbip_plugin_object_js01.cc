#include "content/renderer/cbip_plugin/cbip_plugin_object_js01.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "content/renderer/pepper/message_channel.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/pepper_try_catch.h"
#include "content/renderer/pepper/plugin_module.h"
#include "content/renderer/pepper/v8_var_converter.h"
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

const char kBuiltinMethod01[] = "builtinMethod01";

}  // namespace

CbipPluginObjectJs01::~CbipPluginObjectJs01() {
  if (instance_) {
    instance_->RemoveCbipPluginObject(this);
  }
}

// static
gin::WrapperInfo CbipPluginObjectJs01::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
CbipPluginObjectJs01* CbipPluginObjectJs01::FromV8Object(v8::Isolate* isolate,
    v8::Handle<v8::Object> v8_object) {
  CbipPluginObjectJs01* plugin_object;

  // n.b. if the wrapper info of |v8_object| is not &CbipPluginObjectJs01::kWrapperInfo,
  // gin::ConvertFromV8() fails.

  if (!v8_object.IsEmpty() &&
      gin::ConvertFromV8(isolate, v8_object, &plugin_object)) {
    return plugin_object;
  }
  return NULL;
}

// static
PP_Var CbipPluginObjectJs01::Create(PepperPluginInstanceImpl* instance) {
  V8VarConverter var_converter(instance->pp_instance(),
                               V8VarConverter::kAllowObjectVars);
  PepperTryCatchVar try_catch(instance, &var_converter, NULL);
  CbipPluginObjectJs01* cbip_plugin_object_js01 = new CbipPluginObjectJs01(instance);
  gin::Handle<CbipPluginObjectJs01> object =
      gin::CreateHandle(instance->GetIsolate(), cbip_plugin_object_js01);
  ScopedPPVar result = try_catch.FromV8(object.ToV8());
  DCHECK(!try_catch.HasException());
  return result.Release();
}

v8::Local<v8::Value> CbipPluginObjectJs01::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  {
    VLOG(X_LOG_V) << "ZZZZ CbipPluginObjectJs01::GetNamedProperty:"
                  << " " << identifier
      ;
  }

  if (!instance_)
    return v8::Local<v8::Value>();

  if (identifier == kBuiltinMethod01) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&CbipPluginObjectJs01::BuiltinMethod01,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  return v8::Local<v8::Value>();
}

bool CbipPluginObjectJs01::SetNamedProperty(v8::Isolate* isolate,
    const std::string& identifier,
    v8::Local<v8::Value> value) {
  if (!instance_)
    return false;

  if (identifier == kBuiltinMethod01) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
       isolate, "Cannot set properties with the name builtinMethod01")));
    return true;
  }

  return false;
}

std::vector<std::string> CbipPluginObjectJs01::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  result.push_back(kBuiltinMethod01);

  return result;
}

void CbipPluginObjectJs01::InstanceDeleted() {
  instance_ = NULL;
}

CbipPluginObjectJs01::CbipPluginObjectJs01(PepperPluginInstanceImpl* instance)
    : gin::NamedPropertyInterceptor(instance->GetIsolate(), this),
      instance_(instance),
      weak_ptr_factory_(this) {
  instance_->AddCbipPluginObject(this);
}

gin::ObjectTemplateBuilder CbipPluginObjectJs01::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<CbipPluginObjectJs01>::GetObjectTemplateBuilder(isolate)
      .AddNamedPropertyInterceptor();
}

gin::NamedPropertyInterceptor* CbipPluginObjectJs01::asGinNamedPropertyInterceptor() {
  return this;
}

void CbipPluginObjectJs01::BuiltinMethod01(gin::Arguments* args) {

  assert( instance_->GetIsolate() == args->isolate() );

  v8::Handle<v8::Value> result;

  result = v8::String::NewFromUtf8(
      instance_->GetIsolate(), "XXXXXXX", v8::String::kNormalString, 7);

  args->Return(result);
}

}  // namespace content
