//FIXME: proper copyright.

#include "content/renderer/cbip_plugin/cbip_instance.h"
#include "ppapi/cpp/instance.h"

namespace cbip_pp {

Instance::Instance(PP_Instance instance) : pp_instance_(instance) {
}

Instance::~Instance() {
}

bool Instance::Init(uint32_t /*argc*/, const char* /*argn*/[],
                    const char* /*argv*/[]) {
  return true;
}

}  // namespace cbip_pp
