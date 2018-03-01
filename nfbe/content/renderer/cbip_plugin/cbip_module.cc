//FIXME: proper copyright.

#include "base/logging.h"
#include "content/renderer/cbip_plugin/cbip_module.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/private/ppp_instance_private.h"

#define X_LOG_V 0

namespace cbip_pp {

// -------- ./ppapi/cpp/module.cc

// PPP_Instance implementation -------------------------------------------------

PP_Bool cbip_instif_DidCreate(PP_Instance pp_instance,
                              uint32_t argc,
                              const char* argn[],
                              const char* argv[]) {
  {
    VLOG(X_LOG_V) << "ZZZZ: cbip_instif_DidCreate:"
      ;
  }

#if 0
  Module* module_singleton = Module::Get();
  if (!module_singleton)
    return PP_FALSE;

  Instance* instance = module_singleton->CreateInstance(pp_instance);
  if (!instance)
    return PP_FALSE;
  module_singleton->current_instances_[pp_instance] = instance;
  return PP_FromBool(instance->Init(argc, argn, argv));
#endif

  return PP_TRUE;
}

void cbip_instif_DidDestroy(PP_Instance instance) {
  {
    VLOG(X_LOG_V) << "ZZZZ: cbip_instif_DidDestroy:"
      ;
  }

#if 0
  Module* module_singleton = Module::Get();
  if (!module_singleton)
    return;
  Module::InstanceMap::iterator found =
      module_singleton->current_instances_.find(instance);
  if (found == module_singleton->current_instances_.end())
    return;

  // Remove it from the map before deleting to try to catch reentrancy.
  Instance* obj = found->second;
  module_singleton->current_instances_.erase(found);
  delete obj;
#endif

}

void cbip_instif_DidChangeView(PP_Instance pp_instance, PP_Resource view) {
}

void cbip_instif_DidChangeFocus(PP_Instance pp_instance, PP_Bool has_focus) {
}

PP_Bool cbip_instif_HandleDocumentLoad(PP_Instance pp_instance,
                                       PP_Resource pp_url_loader) {
  return PP_FALSE;
}

static PPP_Instance cbip_instance_interface = {
  &cbip_instif_DidCreate,
  &cbip_instif_DidDestroy,
  &cbip_instif_DidChangeView,
  &cbip_instif_DidChangeFocus,
  &cbip_instif_HandleDocumentLoad
};

// PPP_Instance_Private implementation -----------------------------------------

struct PP_Var cbip_iprivif_GetInstanceObject(PP_Instance pp_instance) {
  /*
   * need to ask the renderer to create.
   */
  return PP_MakeUndefined();
}

static PPP_Instance_Private cbip_instance_private_interface = {
  &cbip_iprivif_GetInstanceObject
};

// Module ----------------------------------------------------------------------

Module::Module() : pp_module_(0), get_browser_interface_(NULL), ppb_core_(NULL) {
  {
    VLOG(X_LOG_V) << "ZZZZ: cbip_pp Module::ctor:"
                  << " this=" << static_cast<void*>(this)
      ;
  }
}

Module::~Module() {
  ppb_core_ = NULL;
}

bool Module::Init() {
  return true;
}

const void* Module::GetPluginInterface(const char* interface_name) {
  if (strcmp(interface_name, PPP_INSTANCE_INTERFACE) == 0)
    return &cbip_instance_interface;
#if 0 /* seems impossible */
  if (strcmp(interface_name, PPP_INSTANCE_PRIVATE_INTERFACE) == 0)
    return &cbip_instance_private_interface;
#endif
  (void)&cbip_instance_private_interface;
  return NULL;
}

bool Module::InternalInit(PP_Module mod,
                          PPB_GetInterface get_browser_interface) {
  {
    VLOG(X_LOG_V) << "ZZZZ: cbip_pp Module::InternalInit:"
                  << " this=" << static_cast<void*>(this)
                  << " module_id=" << mod
      ;
  }

  pp_module_ = mod;
  get_browser_interface_ = get_browser_interface;

  ppb_core_ = reinterpret_cast<const PPB_Core*>(get_browser_interface_(
      PPB_CORE_INTERFACE));
  if (!ppb_core_)
    return false;

  return Init();
}

// -------- ./ppapi/cpp/private/internal_module.cc

static Module* g_module_singleton = NULL;

Module* Module::Get() {
  return g_module_singleton;
}

void InternalSetModuleSingleton(Module* module) {
  g_module_singleton = module;
}

}  // namespace cbip_pp
