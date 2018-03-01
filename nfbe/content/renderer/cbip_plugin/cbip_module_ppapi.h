//FIXME: proper copyright.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_MODULE_PPAPI_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_MODULE_PPAPI_H_

#include "content/renderer/cbip_plugin/cbip_module.h"

namespace cbip_pp_module {

class ModulePpapi : public cbip_pp::Module {
public:
  ModulePpapi();

  ~ModulePpapi() override;

  bool Init() override;

#if 0
  pp::Instance* CreateInstance(PP_Instance pp_instance) override;
#endif

private:
  bool init_was_successful_;
};

}  // namespace cbip_pp_module

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_MODULE_PPAPI_H_
