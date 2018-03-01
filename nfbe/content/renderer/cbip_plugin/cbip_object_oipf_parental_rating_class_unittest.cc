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

class ParentalRatingUnittest : public CbipPluginInstanceUnittest {
 public:
  ParentalRatingUnittest();
  ~ParentalRatingUnittest() override;

  void SetUp() override;
  void TearDown() override;

  scoped_refptr<PepperPluginInstanceImpl> current_instance_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ParentalRatingUnittest);
};

ParentalRatingUnittest::ParentalRatingUnittest() {
}

ParentalRatingUnittest::~ParentalRatingUnittest() {
}


void ParentalRatingUnittest::SetUp() {
  CbipPluginInstanceUnittest::SetUp();
  current_instance_ = getCurrentInstance().get();
}

void ParentalRatingUnittest::TearDown() {
  current_instance_ = nullptr;
  CbipPluginInstanceUnittest::TearDown();
}

TEST_F(ParentalRatingUnittest, testSetNamedProperty) {
  if (!current_instance_)
    return;
  peer::oipf::ParentalRatingItem item;
  CbipObjectOwnerTest obj;
  obj.instance_ = current_instance_;
  v8::Isolate* isolate = current_instance_->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  CbipObjectOipfParentalRatingClass pr_test(&obj, item);
  v8::Local<v8::Value> value;
  EXPECT_TRUE(pr_test.SetNamedProperty(isolate, "name", value));
  EXPECT_TRUE(pr_test.SetNamedProperty(isolate, "scheme", value));
  EXPECT_TRUE(pr_test.SetNamedProperty(isolate, "value", value));
  EXPECT_TRUE(pr_test.SetNamedProperty(isolate, "labels", value));
  EXPECT_TRUE(pr_test.SetNamedProperty(isolate, "region", value));
  EXPECT_FALSE(pr_test.
               SetNamedProperty(isolate, "non_existing_property", value));
}

TEST_F(ParentalRatingUnittest, testGetNamedProperty1) {
  if (!current_instance_)
    return;
  peer::oipf::ParentalRatingItem item;
  CbipObjectOwnerTest obj;
  obj.instance_ = current_instance_;
  v8::Isolate* isolate = current_instance_->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  CbipObjectOipfParentalRatingClass pr_test(&obj, item);
  v8::Local<v8::Value> value;
  EXPECT_STREQ("", (*v8::String::Utf8Value(
               pr_test.GetNamedProperty(isolate, "name"))));
  EXPECT_STREQ("", (*v8::String::Utf8Value(
               pr_test.GetNamedProperty(isolate, "scheme"))));
  EXPECT_EQ(-1,
            pr_test.GetNamedProperty(isolate, "value")->ToInteger()->Value());
  EXPECT_EQ(0,
            pr_test.GetNamedProperty(isolate, "labels")->ToInteger()->Value());
  EXPECT_STREQ("", (*v8::String::Utf8Value(
               pr_test.GetNamedProperty(isolate, "region"))));
}

TEST_F(ParentalRatingUnittest, testGetNamedProperty2) {
  if (!current_instance_)
    return;
  peer::oipf::ParentalRatingItem item("PG-13", "urn:mpeg:mpeg4", 2, 31, "US");
  CbipObjectOwnerTest obj;
  obj.instance_ = current_instance_;
  v8::Isolate* isolate = current_instance_->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  CbipObjectOipfParentalRatingClass pr_test(&obj, item);

  EXPECT_STREQ("PG-13", (*v8::String::Utf8Value(
               pr_test.GetNamedProperty(isolate, "name"))));
  EXPECT_STREQ("urn:mpeg:mpeg4", (*v8::String::Utf8Value(
               pr_test.GetNamedProperty(isolate, "scheme"))));
  EXPECT_EQ(2,
            pr_test.GetNamedProperty(isolate, "value")->ToInteger()->Value());
  EXPECT_EQ(31,
            pr_test.GetNamedProperty(isolate, "labels")->ToInteger()->Value());
  EXPECT_STREQ("US", (*v8::String::Utf8Value(
               pr_test.GetNamedProperty(isolate, "region"))));
}

}  // namespace content
