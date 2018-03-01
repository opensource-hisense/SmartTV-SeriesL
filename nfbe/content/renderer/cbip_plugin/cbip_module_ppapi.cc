//FIXME: proper copyright.

#include "base/logging.h"
#include "content/renderer/cbip_plugin/cbip_module_ppapi.h"
#include "content/renderer/cbip_plugin/cbip_plugin.h"

#define X_LOG_V 0

namespace cbip_pp_module {

ModulePpapi::ModulePpapi() : cbip_pp::Module(),
                             init_was_successful_(false) {
  {
    VLOG(X_LOG_V) << "ZZZZ: cbip ModulePpapi::ctor:"
      ;
  }
}

ModulePpapi::~ModulePpapi() {
  {
    VLOG(X_LOG_V) << "ZZZZ: cbip ModulePpapi::dtor:"
      ;
  }

  if (init_was_successful_) {
    //FIXME: finalize.
  }
}

bool ModulePpapi::Init() {
  {
    VLOG(X_LOG_V) << "ZZZZ: cbip ModulePpapi::Init:"
        ;
  }

  //FIXME: initialize.

  init_was_successful_ = true;
  return true;
}

#if 0

cbip_pp::Instance* ModulePpapi::CreateInstance(PP_Instance pp_instance) {

  {
    VLOG(X_LOG_V) << "ZZZZ: cbip ModulePpapi::CreateInstance"
        ;
  }

  plugin::CbipPlugin* plugin = new plugin::CbipPlugin(pp_instance);

  return plugin;
}

#endif

}  // namespace cbip_pp_module


namespace pp {

#if 0
Module* CreateModule_CBIP() {
  MODULE_PRINTF(("CreateModule ()\n"));
  return new cbip_pp_module::ModulePpapi();
}
#endif

}  // namespace pp
