//FIXME: proper copyright.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_INSTANCE_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_INSTANCE_H_

#include <map>
#include <string>

#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_stdint.h"

namespace cbip_pp {

class Instance {
 public:
  explicit Instance(PP_Instance instance);
  virtual ~Instance();

  PP_Instance pp_instance() const { return pp_instance_; }

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]);

 private:
  PP_Instance pp_instance_;
};

}  // namespace cbip_pp

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_INSTANCE_H_

