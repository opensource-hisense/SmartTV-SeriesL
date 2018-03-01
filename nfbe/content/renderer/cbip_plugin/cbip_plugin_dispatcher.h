// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_DISPATCHER_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_DISPATCHER_H_

#include "base/memory/ref_counted.h"
#include "base/strings/nullable_string16.h"

struct CbipJs00Msg_PT1_Params;

namespace IPC {
class Message;
}

namespace content {

class CbipPluginIPC;

// Dispatches CbipStorage related messages sent to a renderer process from the
// main browser process. There is one instance per child process. Messages
// are dispatched on the main renderer thread. The RenderThreadImpl
// creates an instance and delegates calls to it. This classes also manages
// the application manager.
class CbipPluginDispatcher {
 public:
  CbipPluginDispatcher();
  ~CbipPluginDispatcher();

  scoped_refptr<CbipPluginIPC> GetAppMgr(int cbip_instance_id);
  void ReleaseAppMgr(int cbip_instance_id, CbipPluginIPC *);

  bool OnMessageReceived(const IPC::Message& msg);

  // for sending a reply of sync message sent from the browser.
  // FIXME: friend?
  void Send(IPC::Message* message);

 private:
  class ProxyImpl;

  // IPC message handlers
  void OnAsyncEvent1(const CbipJs00Msg_PT1_Params& params);
  void OnSyncEvent1(const CbipJs00Msg_PT1_Params& params,
                    CbipJs00Msg_PT1_Params* o0);
  void OnAsyncOperationComplete(bool success);

  scoped_refptr<ProxyImpl> proxy_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_DISPATCHER_H_
