// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include <string>
#include "content/renderer/cbip_plugin/test/cbip_plugin_instance_unittest.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object_oipf_parental_control_manager.h"
#include "v8/include/v8.h"

namespace content {

class CbipPluginInstanceUnittest;
class CbipObjectOwnerTest;

class ParentalControlManagerUnittest : public CbipPluginInstanceUnittest {
 public:
  ParentalControlManagerUnittest();
  ~ParentalControlManagerUnittest() override;

  void SetUp() override;
  void TearDown() override;

  scoped_refptr<PepperPluginInstanceImpl> current_instance_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ParentalControlManagerUnittest);
};

ParentalControlManagerUnittest::ParentalControlManagerUnittest() {
}

ParentalControlManagerUnittest::~ParentalControlManagerUnittest() {
}


void ParentalControlManagerUnittest::SetUp() {
  CbipPluginInstanceUnittest::SetUp();
  current_instance_ = getCurrentInstance().get();
}

void ParentalControlManagerUnittest::TearDown() {
  CbipPluginInstanceUnittest::TearDown();
}

TEST_F(ParentalControlManagerUnittest, testParentalControlManagerCreate) {
  if (!current_instance_)
    return;
  v8::HandleScope handle_scope(current_instance_->GetIsolate());
  v8::Local<v8::Context> context = v8::Context::New(current_instance_->GetIsolate());
  v8::Context::Scope contextScope(context);

  CbipPluginObjectParentalControlManager::Create(current_instance_.get());
}

}  // namespace content
