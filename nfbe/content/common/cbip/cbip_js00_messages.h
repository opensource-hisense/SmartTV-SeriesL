#if defined(HBBTV_ON)

#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_param_traits.h"

#define IPC_MESSAGE_START CbipJs00MsgStart

// see statement in include header file "ipc/ipc_message_macros.h"
// regarding export visibility of messages
#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT

IPC_STRUCT_BEGIN(CbipJs00Msg_PT1_Params)
  IPC_STRUCT_MEMBER(base::NullableString16, m0_string)
IPC_STRUCT_END()

// messages sent from the browser to the renderer.

IPC_MESSAGE_CONTROL1(CbipJs00Msg_AsyncEvent1,
                     CbipJs00Msg_PT1_Params)

IPC_SYNC_MESSAGE_CONTROL1_1(CbipJs00Msg_SyncEvent1,
                            CbipJs00Msg_PT1_Params,
                            CbipJs00Msg_PT1_Params)

IPC_MESSAGE_CONTROL1(CbipJs00Msg_AsyncOperationComplete,
                     bool /* success */)

// messages sent from the renderer to the browser.

IPC_MESSAGE_CONTROL1(CbipJs00HostMsg_PT1_Async,
                     CbipJs00Msg_PT1_Params)

IPC_SYNC_MESSAGE_CONTROL1_1(CbipJs00HostMsg_PT1_Sync,
                            CbipJs00Msg_PT1_Params,
                            CbipJs00Msg_PT1_Params)

IPC_MESSAGE_ROUTED1(CbipJs00HostMsg_PT1_Async_Routed,
                    CbipJs00Msg_PT1_Params)

// Used to flush the ipc message queue.
IPC_SYNC_MESSAGE_CONTROL0_0(CbipJs00HostMsg_FlushMessages)

#endif // HBBTV_ON
