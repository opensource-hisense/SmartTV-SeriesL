//FIXME: proper copyright.

#ifndef CBIP_PLUGIN_CBIP_PPAPI_ENTRYPOINTS_H_
#define CBIP_PLUGIN_CBIP_PPAPI_ENTRYPOINTS_H_

#include "ppapi/c/pp_module.h"
#include "ppapi/c/ppb.h"

namespace cbip_plugin {

int PPP_InitializeModule(PP_Module module,
                         PPB_GetInterface get_browser_interface);
const void* PPP_GetInterface(const char* interface_name);
void PPP_ShutdownModule();

}  // namespace cbip_plugin

#endif  // CBIP_PLUGIN_CBIP_PPAPI_ENTRYPOINTS_H_
