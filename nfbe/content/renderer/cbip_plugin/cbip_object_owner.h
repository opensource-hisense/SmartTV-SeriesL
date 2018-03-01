// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OWNER_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OWNER_H_

#include <string>

#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/cbip_plugin/cbip_plugin_ipc.h"
#include "base/memory/weak_ptr.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"

namespace content {

// This serves as a base class for the classes which might own objects that
// have multiple type of owners and that it acts as an Inteface
// that offer GetIsoLate() API

class CbipObjectOwner {
 public:
  virtual ~CbipObjectOwner();

  virtual PepperPluginInstanceImpl* instance() = 0;

  virtual CbipPluginIPC* ipc() = 0;

  virtual v8::Isolate* GetIsolate() = 0;

 protected:
  CbipObjectOwner();

 private:
  DISALLOW_COPY_AND_ASSIGN(CbipObjectOwner);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_OBJECT_OWNER_H_
