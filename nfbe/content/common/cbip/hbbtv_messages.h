#if defined(HBBTV_ON)

#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_param_traits.h"
#include "url/gurl.h"

#define IPC_MESSAGE_START HbbtvMsgStart

// see statement in include header file "ipc/ipc_message_macros.h"
// regarding export visibility of messages
#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT


// messages sent from the renderer to the browser.

// when document is the window.document.
// the target route ID is the RenderFrame of window.document.
IPC_SYNC_MESSAGE_ROUTED0_1(HbbtvHostMsg_GET_OWNER_APPLICATION_0,
                           uint32_t)

// create an application of |uri| and inject it in an application tree.
// |createChild| specifies where in the application tree that the newly
// created application will be inserted (not necessarily be a child of
// |parent_opif_application_index|, nor the tree that
// |parent_opif_application_index| belongs to.)
IPC_SYNC_MESSAGE_ROUTED3_1(HbbtvHostMsg_CREATE_APP,
                           uint32_t /* parent_opif_application_index */,
                           GURL /* uri */,
                           bool /* createChild */,
                           uint32_t)

// destroy an application.
IPC_MESSAGE_ROUTED1(HbbtvHostMsg_DESTROY_APP,
                    uint32_t /* opif_application_index */)

// show the application
IPC_MESSAGE_ROUTED1(HbbtvHostMsg_SHOW_APP,
                    uint32_t /* opif_application_index */)

// hide the application
IPC_MESSAGE_ROUTED1(HbbtvHostMsg_HIDE_APP,
                    uint32_t /* opif_application_index */)

// when a RenderFrame no longer needs an Appliation object,
// the containing RenderProcess sends this message to the browser,
// unless the RenderFrame already received the HOST_DESTROY_APP
// message for that Applicationn object.
IPC_MESSAGE_ROUTED1(HbbtvHostMsg_APP_DELETED,
                    uint32_t /* opif_application_index */)

IPC_MESSAGE_ROUTED1(HbbtvMsg_HOST_APP_LOAD_ERROR,
                    uint32_t /* opif_application_index */)

IPC_MESSAGE_ROUTED2(HbbtvMsg_HOST_APP_SHOWN_OR_HIDDEN,
                    uint32_t /* opif_application_index */,
                    bool /* is_shown */)

// when the browser deleted an Application, it broadcast this
// message to all the RenderFrames which refers to the deleted
// Application but does not yet send APP_DELETED message.
// the RenderFrame which receives this message generates
// ApplicationDestroyRequest event, and unref the Application
// object so that it is later garbage collected.
IPC_MESSAGE_ROUTED1(HbbtvMsg_HOST_APP_DELETED,
                    uint32_t /* opif_application_index */)

// ApplicationPrivate.getFreeMem()
IPC_SYNC_MESSAGE_ROUTED0_1(HbbtvHostMsg_GET_FREE_MEM,
                           double)

#endif
