//FIXME: proper copyright.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_MODULE_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_MODULE_H_

#include <map>
#include <string>

#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/ppb.h"
#include "ppapi/c/ppb_core.h"

namespace cbip_pp {

class Instance;

class Module {
 public:
  typedef std::map<PP_Instance, Instance*> InstanceMap;

  Module();
  virtual ~Module();

  static Module* Get();

  virtual bool Init();

  const void* GetPluginInterface(const char* interface_name);

  bool InternalInit(PP_Module mod,
                    PPB_GetInterface get_browser_interface);

#if 0
 protected:
  virtual Instance* CreateInstance(PP_Instance instance) = 0;
#endif

 private:
  // Unimplemented (disallow copy and assign).
  Module(const Module&);
  Module& operator=(const Module&);

  PP_Module pp_module_;
  PPB_GetInterface get_browser_interface_;

  const PPB_Core* ppb_core_;
};

}  // namespace cbip_pp

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_MODULE_H_


#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_INTERNAL_MODULE_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_INTERNAL_MODULE_H_

namespace cbip_pp {
class Module;
void InternalSetModuleSingleton(Module* module);
}  // namespace cbip_pp

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_INTERNAL_MODULE_H_
