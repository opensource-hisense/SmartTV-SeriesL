//FIXME: proper copyright.

#include "base/logging.h"
#include "content/renderer/cbip_plugin/cbip_module.h"
#include "content/renderer/cbip_plugin/cbip_module_ppapi.h"
#include "content/renderer/cbip_plugin/cbip_ppapi_entrypoints.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/ppb.h"

#define X_LOG_V 0

namespace cbip_plugin {

int32_t PPP_InitializeModule(PP_Module module_id,
                             PPB_GetInterface get_browser_interface) {
  {
    VLOG(X_LOG_V) << "ZZZZ: cbip_plugin PPP_InitializeModule:"
		  << " module_id=" << module_id;
      ;
  }

  cbip_pp_module::ModulePpapi* module = new cbip_pp_module::ModulePpapi();
  if (!module->InternalInit(module_id, get_browser_interface)) {
    delete module;
    return PP_ERROR_FAILED;
  }

  cbip_pp::InternalSetModuleSingleton(module);
  return PP_OK;
}

void PPP_ShutdownModule() {
  {
    VLOG(X_LOG_V) << "ZZZZ: cbip_plugin PPP_ShutdownModule:"
      ;
  }

  delete cbip_pp::Module::Get();
  cbip_pp::InternalSetModuleSingleton(NULL);
}

const void* PPP_GetInterface(const char* interface_name) {
  {
    VLOG(X_LOG_V) << "ZZZZ: cbip_plugin PPP_GetInterface:"
		  << " " << interface_name
      ;
  }

  if (!cbip_pp::Module::Get())
    return NULL;
  return cbip_pp::Module::Get()->GetPluginInterface(interface_name);
}

}  // namespace nacl_plugin
