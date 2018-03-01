// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_CONSTANTS_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_CONSTANTS_H_

/* FIXME: should be in content/common/cbip/ */

#include "base/files/file_path.h"

namespace cbip {

extern const char kCbipPluginName[];
extern const char kCbipJs00PluginMimeType[];
extern const char kCbipJs00PluginExtension[];
extern const char kCbipJs00PluginDescription[];
extern const char kCbipJs01PluginMimeType[];
extern const char kCbipJs01PluginExtension[];
extern const char kCbipJs01PluginDescription[];
extern const char kCbipOipfApplicationManagerPluginMimeType[];
extern const char kCbipOipfApplicationManagerPluginExtension[];
extern const char kCbipOipfApplicationManagerPluginDescription[];
extern const char kCbipOipfVideoBoradcastPluginMimeType[];
extern const char kCbipOipfVideoBoradcastPluginExtension[];
extern const char kCbipOipfVideoBoradcastPluginDescription[];
extern const char kOipfConfigurationPluginMimeType[];
extern const char kOipfConfigurationPluginExtension[];
extern const char kOipfConfigurationPluginDescription[];
extern const char kOipfParentalControlManagerPluginMimeType[];
extern const char kOipfParentalControlManagerPluginExtension[];
extern const char kOipfParentalControlManagerPluginDescription[];
extern const char kOipfCapabilitiesPluginMimeType[];
extern const char kOipfCapabilitiesPluginExtension[];
extern const char kOipfCapabilitiesPluginDescription[];

extern const base::FilePath::CharType kInternalCbipPluginFileName[];

}  // namespace cbip

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_CONSTANTS_H_
