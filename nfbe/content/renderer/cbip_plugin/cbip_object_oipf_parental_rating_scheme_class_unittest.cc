// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include <string>
#include <vector>
#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "content/common/frame_messages.h"
#include "content/public/test/test_utils.h"
#include "content/renderer/cbip_plugin/cbip_constants.h"
#include "content/renderer/cbip_plugin/test/cbip_plugin_instance_unittest.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_parental_rating_scheme_class.h"
#include "content/renderer/cbip_plugin/cbip_object_oipf_parental_rating_class.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/plugin_module.h"
#include "content/renderer/render_view_impl.h"
#include "content/renderer/render_frame_impl.h"
#include "content/public/test/render_view_test.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/ppapi_permissions.h"
#include "v8/include/v8.h"

namespace content {

class CbipPluginInstanceUnittest;
class CbipObjectOwnerTest;

class ParentalRatingSchemeUnittest : public CbipPluginInstanceUnittest {
 public:
  ParentalRatingSchemeUnittest();
  ~ParentalRatingSchemeUnittest() override;

  void SetUp() override;
  void TearDown() override;

  scoped_refptr<PepperPluginInstanceImpl> current_instance_;
  std::string test_name_;
  peer::oipf::ParentalRatingItem test_threshold_;
  std::vector<peer::oipf::ParentalRatingSchemeValue>
      test_pr_scheme_value_items_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ParentalRatingSchemeUnittest);
};

ParentalRatingSchemeUnittest::ParentalRatingSchemeUnittest() {
}

ParentalRatingSchemeUnittest::~ParentalRatingSchemeUnittest() {
}

void ParentalRatingSchemeUnittest::SetUp() {
  CbipPluginInstanceUnittest::SetUp();

  current_instance_ = getCurrentInstance().get();

  test_name_ = "urn:oipf:GermanyFSKCS";
  peer::oipf::ParentalRatingItem
      threshold("16", "urn:oipf:GermanyFSKCS", 3, 15, "DE");
  test_threshold_ = threshold;

  peer::oipf::ParentalRatingSchemeValue item("10", "image/10.png");
  test_pr_scheme_value_items_.push_back(item);
  peer::oipf::ParentalRatingSchemeValue item1("16", "image/16.png");
  test_pr_scheme_value_items_.push_back(item1);
}

void ParentalRatingSchemeUnittest::TearDown() {
  current_instance_ = nullptr;
  CbipPluginInstanceUnittest::TearDown();
}

TEST_F(ParentalRatingSchemeUnittest, testSetNamedProperty) {
  if (!current_instance_)
    return;
  v8::Isolate* isolate = current_instance_->GetIsolate();

  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      v8::Context::New(current_instance_->GetIsolate());
  v8::Context::Scope contextScope(context);

  CbipObjectOwnerTest obj;
  obj.instance_ = current_instance_;

  peer::oipf::ParentalRatingScheme item;
  CbipObjectOipfParentalRatingSchemeClass pr_test(&obj, item);
  v8::Local<v8::Value> value;
  EXPECT_TRUE(pr_test.SetNamedProperty(isolate, "name", value));
  EXPECT_TRUE(pr_test.SetNamedProperty(isolate, "threshold", value));
  EXPECT_TRUE(pr_test.SetNamedProperty(isolate, "length", value));
  EXPECT_FALSE(pr_test.
               SetNamedProperty(isolate, "non_existing_property", value));
}

TEST_F(ParentalRatingSchemeUnittest, testGetNamedProperty1) {
  if (!current_instance_)
    return;
  v8::HandleScope handle_scope(current_instance_->GetIsolate());
  v8::Local<v8::Context> context =
      v8::Context::New(current_instance_->GetIsolate());
  v8::Context::Scope contextScope(context);

  CbipObjectOwnerTest obj;
  obj.instance_ = current_instance_;
  v8::Isolate* isolate = current_instance_->GetIsolate();

  peer::oipf::ParentalRatingScheme scheme;
  CbipObjectOipfParentalRatingSchemeClass pr_test(&obj, scheme);
  EXPECT_STREQ("", (*v8::String::Utf8Value(
               pr_test.GetNamedProperty(isolate, "name"))));
  v8::Local<v8::Value> v8_value =
      pr_test.GetNamedProperty(isolate, "threshold");

  CbipObjectOipfParentalRatingClass *ptr =
      CbipObjectOipfParentalRatingClass::FromV8Object(isolate,
          v8_value->ToObject());
  peer::oipf::ParentalRatingItem value;
  CbipObjectOipfParentalRatingClass::TestConvertToStruct(ptr, &value);
  EXPECT_TRUE(scheme.threshold == value);

  EXPECT_EQ(0, pr_test.GetNamedProperty(isolate, "length")->Int32Value());
}

TEST_F(ParentalRatingSchemeUnittest, testGetNamedProperty2) {
  if (!current_instance_)
    return;
  v8::Isolate* isolate = current_instance_->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      v8::Context::New(current_instance_->GetIsolate());
  v8::Context::Scope contextScope(context);

  CbipObjectOwnerTest obj;
  obj.instance_ = current_instance_;

  peer::oipf::ParentalRatingScheme scheme;
  scheme.name = test_name_;
  scheme.threshold = test_threshold_;
  scheme.parental_rating_scheme_items = test_pr_scheme_value_items_;
  CbipObjectOipfParentalRatingSchemeClass pr_test(&obj, scheme);
  EXPECT_STREQ(test_name_.c_str(), (*v8::String::Utf8Value(
               pr_test.GetNamedProperty(isolate, "name"))));
  v8::Local<v8::Value> v8_value =
      pr_test.GetNamedProperty(isolate, "threshold");
  CbipObjectOipfParentalRatingClass *ptr =
      CbipObjectOipfParentalRatingClass::FromV8Object(isolate,
          v8_value->ToObject());
  peer::oipf::ParentalRatingItem value;
  CbipObjectOipfParentalRatingClass::TestConvertToStruct(ptr, &value);
  EXPECT_TRUE(scheme.threshold == value);

  if (test_name_ == "dvb-si")
    EXPECT_EQ(0, pr_test.GetNamedProperty(isolate, "length")->Int32Value());
  else
    EXPECT_EQ(scheme.parental_rating_scheme_items.size(),
              pr_test.GetNamedProperty(isolate, "length")->Uint32Value());
}

TEST_F(ParentalRatingSchemeUnittest, testIndexOf) {
  if (!current_instance_)
    return;
  v8::Isolate* isolate = current_instance_->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      v8::Context::New(current_instance_->GetIsolate());
  v8::Context::Scope contextScope(context);

  CbipObjectOwnerTest obj;
  obj.instance_ = current_instance_;

  peer::oipf::ParentalRatingScheme scheme;
  scheme.name = test_name_;
  scheme.threshold = test_threshold_;
  scheme.parental_rating_scheme_items = test_pr_scheme_value_items_;
  CbipObjectOipfParentalRatingSchemeClass pr_test(&obj, scheme);

  v8::Local<v8::Function> cb =
      v8::Local<v8::Function>::Cast(
          pr_test.GetNamedProperty(isolate, "indexOf"));
  const unsigned int argc = 1;
  v8::Handle<v8::Value> argv[argc] ={ v8::String::NewFromUtf8(isolate, "16") };
  v8::Handle<v8::Value> js_result = cb->Call(v8::Null(isolate), argc, argv);
  EXPECT_TRUE(js_result->IsInt32());
  EXPECT_EQ(1 , js_result->IntegerValue());

  // non existing scheme value name
  argv[0] = v8::String::NewFromUtf8(isolate, "0");
  js_result = cb->Call(v8::Null(isolate), argc, argv);
  EXPECT_TRUE(js_result->IsInt32());
  EXPECT_EQ(-1 , js_result->IntegerValue());

  // invalid input
  argv[0] = v8::Integer::New(isolate, 1);
  js_result = cb->Call(v8::Null(isolate), argc, argv);
  EXPECT_TRUE(js_result->IsInt32());
  EXPECT_EQ(-1 , js_result->IntegerValue());
}

TEST_F(ParentalRatingSchemeUnittest, testIconUri) {
  if (!current_instance_)
    return;
  v8::Isolate* isolate = current_instance_->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      v8::Context::New(current_instance_->GetIsolate());
  v8::Context::Scope contextScope(context);

  CbipObjectOwnerTest obj;
  obj.instance_ = current_instance_;

  peer::oipf::ParentalRatingScheme scheme;
  scheme.name = test_name_;
  scheme.threshold = test_threshold_;
  scheme.parental_rating_scheme_items = test_pr_scheme_value_items_;
  CbipObjectOipfParentalRatingSchemeClass pr_test(&obj, scheme);

  v8::Local<v8::Function> cb =
      v8::Local<v8::Function>::Cast(
          pr_test.GetNamedProperty(isolate, "iconUri"));
  const unsigned int argc = 1;
  v8::Handle<v8::Value> argv[argc] ={ v8::Integer::New(isolate, 1) };
  v8::Handle<v8::Value> js_result = cb->Call(v8::Null(isolate), argc, argv);
  EXPECT_TRUE(js_result->IsString());
  EXPECT_EQ("image/16.png", gin::V8ToString(js_result));

  // non existing index
  argv[0] = v8::Integer::New(isolate, 10);
  js_result = cb->Call(v8::Null(isolate), argc, argv);
  EXPECT_TRUE(js_result->IsUndefined());

  // invalid input
  argv[0] = v8::String::NewFromUtf8(isolate, "0");
  js_result = cb->Call(v8::Null(isolate), argc, argv);
  EXPECT_TRUE(js_result->IsUndefined());
}

TEST_F(ParentalRatingSchemeUnittest, testItem) {
  if (!current_instance_)
    return;
  v8::Isolate* isolate = current_instance_->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      v8::Context::New(current_instance_->GetIsolate());
  v8::Context::Scope contextScope(context);

  CbipObjectOwnerTest obj;
  obj.instance_ = current_instance_;

  peer::oipf::ParentalRatingScheme scheme;
  scheme.name = test_name_;
  scheme.threshold = test_threshold_;
  scheme.parental_rating_scheme_items = test_pr_scheme_value_items_;
  CbipObjectOipfParentalRatingSchemeClass pr_test(&obj, scheme);

  v8::Local<v8::Function> cb =
      v8::Local<v8::Function>::Cast(
          pr_test.GetNamedProperty(isolate, "item"));
  const unsigned int argc = 1;
  v8::Handle<v8::Value> argv[argc] ={ v8::Integer::New(isolate, 1) };
  v8::Handle<v8::Value> js_result = cb->Call(v8::Null(isolate), argc, argv);
  EXPECT_TRUE(js_result->IsString());
  EXPECT_EQ("16", gin::V8ToString(js_result));

  argv[0] = v8::Integer::New(isolate, 0);
  js_result = cb->Call(v8::Null(isolate), argc, argv);
  EXPECT_EQ("10", gin::V8ToString(js_result));

  // non existing index
  argv[0] = v8::Integer::New(isolate, 10);
  js_result = cb->Call(v8::Null(isolate), argc, argv);
  EXPECT_TRUE(js_result->IsUndefined());

  // invalid input
  argv[0] = v8::String::NewFromUtf8(isolate, "0");
  js_result = cb->Call(v8::Null(isolate), argc, argv);
  EXPECT_TRUE(js_result->IsUndefined());

  argv[0] = v8::Boolean::New(isolate, true);
  js_result = cb->Call(v8::Null(isolate), argc, argv);
  EXPECT_TRUE(js_result->IsUndefined());
}
}  // namespace content
