// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_IPC_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_IPC_H_

#include <string>
#include "access/content/browser/cbip_plugin_host/oipf_message_handler.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"

class GURL;
struct ConfigurationParam;

namespace peer {
namespace oipf {
struct VideoBroadcastChannel;
}
}

namespace content {

class CbipPluginProxy;

class CONTENT_EXPORT CbipPluginIPC
    : public base::RefCounted<CbipPluginIPC> {
 public:
  CbipPluginIPC(CbipPluginProxy* proxy);

  void PT1Async(const base::string16& p0);

  base::NullableString16 PT1Sync(const base::string16& p0);

  void PT1AsyncRouted(int routing_id, const base::string16& p0);

  // HbbtvHostMsg_GET_OWNER_APPLICATION_0
  uint32_t GetOwnerApplication0(int routing_id);

  // HbbtvHostMsg_CREATE_APP
  uint32_t CreateApplication(int routing_id,
                             uint32_t parent_oipf_application_index,
                             const GURL& uri,
                             bool createChild);

  // HbbtvHostMsg_DESTROY_APP
  void DestroyApplication(int routing_id,
                          uint32_t oipf_application_index);

  // HbbtvHostMsg_SHOW_APP
  void Show(int routing_id, uint32_t oipf_application_index);

  // HbbtvHostMsg_HIDE_APP
  void Hide(int routing_id, uint32_t oipf_application_index);

  // HbbtvHostMsg_APP_DELETED
  void ApplicationDeleted(int routing_id, uint32_t oipf_application_index);

  // HbbtvHostMsg_GET_FREE_MEM
  double GetFreeMem(int routing_id);

  //ConfigurationHostMsg_FillConfigurationParam
  void GetConfigurationData(int routing_id, ConfigurationParam* configuration_param);

  //ConfigurationHostMsg_FillConfigurationLocalSystemParam
  void GetConfigurationLocalSystem(int routing_id, ConfigurationLocalSysParam* configuration_local_sys_param);

  // VideoBroadcastHostMsg_Bind
  void Bind(int routing_id);

  // VideoBroadcastHostMsg_Unbind
  void Unbind(int routing_id);

  // VideoBroadcastHostMsg_SetFullScreen
  void SetFullScreen(int routing_id, bool is_full_screen);

  // VideoBroadcastHostMsg_GetPlayState
  void GetPlayState(int routing_id, int* play_state);

  // VideoBroadcastHostMsg_GetCurrentChannel
  void GetCurrentChannel(int routing_id, bool* is_success, peer::oipf::VideoBroadcastChannel* channel);

  // VideoBroadcastHostMsg_SetVolume
  void SetVolume(int routing_id, int volume);

  // VideoBroadcastHostMsg_GetVolume
  void GetVolume(int routing_id, int* volume);

  // VideoBroadcastHostMsg_BindToCurrentChannel
  void BindToCurrentChannel(int routing_id);

  // VideoBroadcastHostMsg_CreateChannelObjectSiDirect
  void CreateChannelObject(
      int routing_id, const VideoBroadcastCreateChannelObjectSiDirectParam& param,
      bool* is_success,
      peer::oipf::VideoBroadcastChannel* channel);

  // VideoBroadcastHostMsg_CreateChannelObjectOther
  void CreateChannelObject(
      int routing_id, const VideoBroadcastCreateChannelObjectOtherParam& param,
      bool* is_success,
      peer::oipf::VideoBroadcastChannel* channel);

  // VideoBroadcastHostMsg_SetChannel
  void SetChannel(int routing_id, const peer::oipf::VideoBroadcastChannel& channel, bool trickplay, const std::string& url);

  // VideoBroadcastHostMsg_PrevChannel
  void PrevChannel(int routing_id);

  // VideoBroadcastHostMsg_NextChannel
  void NextChannel(int routing_id);

  // VideoBroadcastHostMsg_Release
  void VideoBroadcastRelease(int routing_id);

  // VideoBroadcastHostMsg_Stop
  void Stop(int routing_id);

  // VideoBroadcastHostMsg_GetComponents
  void GetComponents(int routing_id, int component_type, bool* is_success,
                     peer::oipf::AVComponentCollection* component_collection);

  // VideoBroadcastHostMsg_GetCurrentActiveComponents
  void GetCurrentActiveComponents(int routing_id, int component_type,
                                  bool* is_success,
                                  peer::oipf::AVComponentCollection* compoent_collection);

  // VideoBroadcastHostMsg_SelectComponent
  void SelectComponent(int routing_id, int component_type,
                       peer::oipf::AVComponentsData* av_components_data);

  // VideoBroadcastHostMsg_UnselectComponent
  void UnselectComponent(int routing_id, int component_type,
                         peer::oipf::AVComponentsData* av_components_data);

  // VideoBroadcastHostMsg_DidChangeView
  void DidChangeView(int routing_id, const gfx::Rect& rect);

  //VideoBroadcastHostMsg_GetNumProgrammess  
  int GetNumProgrammes(int routing_id);

  //VideoBroadcastHostMsg_GetProgrammeItemByIndex
  void GetProgrammeItemByIndex(int routing_id, int index, bool* is_success, peer::oipf::VideoBroadcastProgramme* channel);  

  //VideoBroadcastHostMsg_GetProgrammeSIDescriptors
  void GetProgrammeSIDescriptors(int routing_id, std::string programme_id_, 
      int descriptor_tag, int descriptor_tag_extension, int private_data_specifier, bool* is_success, 
      peer::oipf::StringCollectionData* programme_string_collection);

  // ParentalControlManagerHostMsg_Bind
  void BindParentalControlManager(int routing_id);

  // ParentalControlManagerHostMsg_Unbind
  void UnbindParentalControlManager(int routing_id);

  // ParentalControlManagerHostMsg_GetNumberOfParentalRatingSchemes
  int GetNumOfParentralRatingSchemes(int routing_id);

  // ParentalControlManagerHostMsg_GetParentalRatingSchemeByIndex
  void GetParentalRatingSchemeByIndex(int routing_id, uint32_t index,
      bool* is_success,
      peer::oipf::ParentalRatingScheme* parental_rating_scheme);

  // ParentalControlManagerHostMsg_GetParentalRatingSchemeByName
  void GetParentalRatingSchemeByName(int routing_id, const std::string& name,
      bool* is_success,
      peer::oipf::ParentalRatingScheme* parental_rating_scheme);

  // ParentalRatingCollectionHostMsg_AddParentalRating
  void AddParentalRating(int routing_id,
      peer::oipf::ParentalRatingItem* parental_rating);

  // OIPFCapabilitiesHostMsg_Bind
  void BindOipfCapabilities(int routing_id);
  // OIPFCapabilitiesHostMsg_Unbind
  void UnbindOipfCapabilities(int routing_id);
  // OIPFCapabilitiesHostMsg_GetXmlCapabilities
  void GetXmlCapabilities(int routing_id, std::string* xml_capabilities);
  // OIPFCapabilitiesHostMsg_GetExtraSDVideoDecodes
  void GetExtraSDVideoDecodes(int routing_id, int32_t* extra_sd_video_decodes);
  // OIPFCapabilitiesHostMsg_GetExtraHDVideoDecodes
  void GetExtraHDVideoDecodes(int routing_id, int32_t* extra_hd_video_decodes);
  // OIPFCapabilitiesHostMsg_HasCapabilities
  bool HasCapabilities(int routing_id, const std::string& profile_name);
 private:
  friend class base::RefCounted<CbipPluginIPC>;
  ~CbipPluginIPC();

  // Async completion callbacks.
  void OnPT1AsyncComplete(bool success);

  scoped_refptr<CbipPluginProxy> proxy_;
  base::WeakPtrFactory<CbipPluginIPC> weak_factory_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_IPC_H_
