//FIXME: proper copyright.

#include <sys/stat.h>
#include <sys/types.h>

#include <string>

#include "base/logging.h"
#include "content/renderer/cbip_plugin/cbip_plugin.h"
#include "ppapi/c/pp_errors.h"

namespace cbip_plugin {

CbipPlugin::CbipPlugin(PP_Instance pp_instance) : cbip_pp::Instance(pp_instance) {
}

CbipPlugin::~CbipPlugin() {
}

}  // namespace plugin
