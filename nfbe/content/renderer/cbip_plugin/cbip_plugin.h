//FIXME: proper copyright.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_H_

#include <stdio.h>

#include <string>

#include "content/renderer/cbip_plugin/cbip_instance.h"
#include "ppapi/c/pp_instance.h"

namespace cbip_plugin {

class CbipPlugin : public cbip_pp::Instance {
 public:
  explicit CbipPlugin(PP_Instance instance);

 private:
  DISALLOW_COPY_AND_ASSIGN(CbipPlugin);
  ~CbipPlugin() override;
};

}  // namespace plugin

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_H_
