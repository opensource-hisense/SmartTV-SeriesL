// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "content/renderer/cbip_plugin/cbip_plugin_ipc.h"

#include "access/peer/include/plugin/dae/video_broadcast_plugin.h"
#include "content/renderer/cbip_plugin/cbip_plugin_proxy.h"

namespace content {

CbipPluginIPC::CbipPluginIPC(CbipPluginProxy* proxy)
    : proxy_(proxy),
      weak_factory_(this) {}

CbipPluginIPC::~CbipPluginIPC() {}

/* async IPC which does not need async ack */
/* FIXME: impl */

/* async IPC which needs async ack */
void CbipPluginIPC::PT1Async(const base::string16& p0) {
  proxy_->PT1Async(p0,
                   base::Bind(&CbipPluginIPC::OnPT1AsyncComplete,
                              weak_factory_.GetWeakPtr()));
}

/* sync IPC which does not need async ack */
base::NullableString16 CbipPluginIPC::PT1Sync(const base::string16& p0) {
  return proxy_->PT1Sync(p0);
}

/* async IPC which does not need async ack, routed */
void CbipPluginIPC::PT1AsyncRouted(int routing_id, const base::string16& p0) {
  proxy_->PT1AsyncRouted(routing_id, p0);
}

/* sync IPC which needs async ack */
/* FIXME: impl */

// HbbtvHostMsg_GET_OWNER_APPLICATION_0
uint32_t CbipPluginIPC::GetOwnerApplication0(int routing_id) {
  return proxy_->GetOwnerApplication0(routing_id);
}

// HbbtvHostMsg_CREATE_APP
uint32_t CbipPluginIPC::CreateApplication(int routing_id,
    uint32_t parent_oipf_application_index,
    const GURL& uri,
    bool createChild) {
  return proxy_->CreateApplication(routing_id,
      parent_oipf_application_index, uri, createChild);
}

// HbbtvHostMsg_DESTROY_APP
void CbipPluginIPC::DestroyApplication(int routing_id,
    uint32_t oipf_application_index) {
  proxy_->DestroyApplication(routing_id, oipf_application_index);
}

// HbbtvHostMsg_SHOW_APP
void CbipPluginIPC::Show(int routing_id,
    uint32_t oipf_application_index) {
  proxy_->Show(routing_id, oipf_application_index);
}

// HbbtvHostMsg_HIDE_APP
void CbipPluginIPC::Hide(int routing_id,
    uint32_t oipf_application_index) {
  proxy_->Hide(routing_id, oipf_application_index);
}

// HbbtvHostMsg_APP_DELETED
void CbipPluginIPC::ApplicationDeleted(int routing_id,
    uint32_t oipf_application_index) {
  proxy_->ApplicationDeleted(routing_id, oipf_application_index);
}

// HbbtvHostMsg_GET_FREE_MEM
double CbipPluginIPC::GetFreeMem(int routing_id) {
  return proxy_->GetFreeMem(routing_id);
}

//ConfigurationHostMsg_FillConfigurationParam
void  CbipPluginIPC::GetConfigurationData(int routing_id, ConfigurationParam* configuration_param) {  
  proxy_->GetConfigurationData(routing_id, configuration_param);
}

//ConfigurationHostMsg_FillConfigurationLocalSystemParam
void  CbipPluginIPC::GetConfigurationLocalSystem(int routing_id, ConfigurationLocalSysParam* configuration_local_sys_param) {
  proxy_->GetConfigurationLocalSystem(routing_id, configuration_local_sys_param);
}

// VideoBroadcastHostMsg_Bind
void  CbipPluginIPC::Bind(int routing_id) {
  proxy_->Bind(routing_id);
}

// VideoBroadcastHostMsg_Unbind
void  CbipPluginIPC::Unbind(int routing_id) {
  proxy_->Unbind(routing_id);
}

// VideoBroadcastHostMsg_SetFullScreen
void  CbipPluginIPC::SetFullScreen(int routing_id, bool is_full_screen) {
  proxy_->SetFullScreen(routing_id, is_full_screen);
}

// VideoBroadcastHostMsg_GetPlayState
void  CbipPluginIPC::GetPlayState(int routing_id, int* play_state) {
  proxy_->GetPlayState(routing_id, play_state);
}

// VideoBroadcastHostMsg_GetCurrentChannel
void  CbipPluginIPC::GetCurrentChannel(int routing_id, bool* is_success, peer::oipf::VideoBroadcastChannel* channel) {
  proxy_->GetCurrentChannel(routing_id, is_success, channel);
}

// VideoBroadcastHostMsg_SetVolume
void  CbipPluginIPC::SetVolume(int routing_id, int volume) {
  proxy_->SetVolume(routing_id, volume);
}

// VideoBroadcastHostMsg_GetVolume
void  CbipPluginIPC::GetVolume(int routing_id, int* volume) {
  proxy_->GetVolume(routing_id, volume);
}

// VideoBroadcastHostMsg_BindToCurrentChannel
void  CbipPluginIPC::BindToCurrentChannel(int routing_id) {
  proxy_->BindToCurrentChannel(routing_id);
}

// VideoBroadcastHostMsg_CreateChannelObjectSiDirect
void  CbipPluginIPC::CreateChannelObject(
    int routing_id,
    const VideoBroadcastCreateChannelObjectSiDirectParam& param,
    bool* is_success,
    peer::oipf::VideoBroadcastChannel* channel) {
  proxy_->CreateChannelObject(routing_id, param, is_success, channel);
}

// VideoBroadcastHostMsg_CreateChannelObjectOther
void  CbipPluginIPC::CreateChannelObject(
    int routing_id,
    const VideoBroadcastCreateChannelObjectOtherParam& param,
    bool* is_success,
    peer::oipf::VideoBroadcastChannel* channel) {
  proxy_->CreateChannelObject(routing_id, param, is_success, channel);
}

// VideoBroadcastHostMsg_SetChannel
void  CbipPluginIPC::SetChannel(int routing_id, const peer::oipf::VideoBroadcastChannel& channel, bool trickplay, const std::string& url) {
  proxy_->SetChannel(routing_id, channel, trickplay, url);
}

// VideoBroadcastHostMsg_PrevChannel
void  CbipPluginIPC::PrevChannel(int routing_id) {
  proxy_->PrevChannel(routing_id);
}

// VideoBroadcastHostMsg_NextChannel
void  CbipPluginIPC::NextChannel(int routing_id) {
  proxy_->NextChannel(routing_id);
}

// VideoBroadcastHostMsg_Stop
void  CbipPluginIPC::Stop(int routing_id) {
  proxy_->Stop(routing_id);
}

// VideoBroadcastHostMsg_GetComponents
void CbipPluginIPC::GetComponents(
    int routing_id,
    int component_type,
    bool* is_success,
    peer::oipf::AVComponentCollection* component_collection) {
  proxy_->GetComponents(routing_id, component_type, is_success, component_collection);
}

// VideoBroadcastHostMsg_GetCurrentActiveComponents
void CbipPluginIPC::GetCurrentActiveComponents(
    int routing_id,
    int component_type,
    bool* is_success,
    peer::oipf::AVComponentCollection* compoent_collection) {
  proxy_->GetCurrentActiveComponents(routing_id, component_type, is_success, compoent_collection);
}

// VideoBroadcastHostMsg_SelectComponent
void CbipPluginIPC::SelectComponent(
    int routing_id,
    int component_type,
    peer::oipf::AVComponentsData* av_components_data) {
  proxy_->SelectComponent(routing_id, component_type, av_components_data);
}

// VideoBroadcastHostMsg_UnselectComponent
void CbipPluginIPC::UnselectComponent(
    int routing_id,
    int component_type,
    peer::oipf::AVComponentsData* av_components_data) {
  proxy_->UnselectComponent(routing_id, component_type, av_components_data);
}

// VideoBroadcastHostMsg_Release
void  CbipPluginIPC::VideoBroadcastRelease(int routing_id) {
  proxy_->VideoBroadcastRelease(routing_id);
}

// VideoBroadcastHostMsg_DidChangeView
void CbipPluginIPC::DidChangeView(int routing_id, const gfx::Rect& rect) {
  proxy_->DidChangeView(routing_id, rect);
}

// VideoBroadcastHostMsg_GetNumProgrammes  
int CbipPluginIPC::GetNumProgrammes(int routing_id) {
  return proxy_->GetNumProgrammes(routing_id );
}

// VideoBroadcastHostMsg_GetProgrammeItemByIndex
void CbipPluginIPC::GetProgrammeItemByIndex(int routing_id, int index, bool* is_success, peer::oipf::VideoBroadcastProgramme* programme) {
  proxy_->GetProgrammeItemByIndex(routing_id, index, is_success, programme);
}

// VideoBroadcastHostMsg_GetProgrammeSIDescriptors
void CbipPluginIPC::GetProgrammeSIDescriptors(int routing_id, std::string programme_id, 
    int descriptor_tag, int descriptor_tag_extension, int private_data_specifier, bool* is_success, 
    peer::oipf::StringCollectionData* programme_string_collection) {
  proxy_->GetProgrammeSIDescriptors(routing_id, programme_id, descriptor_tag, descriptor_tag_extension,
          private_data_specifier, is_success, programme_string_collection);
}

void CbipPluginIPC::OnPT1AsyncComplete(bool success) {
  DCHECK(success);
}

// ParentalControlManagerHostMsg_Bind
void CbipPluginIPC::BindParentalControlManager(int routing_id) {
  proxy_->BindParentalControlManager(routing_id);
}

// ParentalControlManagerHostMsg_Unbind
void CbipPluginIPC::UnbindParentalControlManager(int routing_id) {
  proxy_->UnbindParentalControlManager(routing_id);
}

// ParentalControlManagerHostMsg_GetNumberOfParentalRatingSchemes
int CbipPluginIPC::GetNumOfParentralRatingSchemes(int routing_id) {
  return proxy_->GetNumOfParentralRatingSchemes(routing_id);
}
// ParentalControlManagerHostMsg_GetParentalRatingSchemeByIndex
void CbipPluginIPC::GetParentalRatingSchemeByIndex(int routing_id,
    uint32_t index,
    bool* is_success,
    peer::oipf::ParentalRatingScheme* parental_rating_scheme) {
  proxy_->GetParentalRatingSchemeByIndex(routing_id,
                                         index,
                                         is_success,
                                         parental_rating_scheme);
}

// ParentalControlManagerHostMsg_GetParentalRatingSchemeByName
void CbipPluginIPC::GetParentalRatingSchemeByName(int routing_id,
    const std::string& name,
    bool* is_success,
    peer::oipf::ParentalRatingScheme* parental_rating_scheme) {
  proxy_->GetParentalRatingSchemeByName(routing_id,
                                         name,
                                         is_success,
                                         parental_rating_scheme);
}

// ParentalRatingCollectionHostMsg_AddParentalRating
void CbipPluginIPC::AddParentalRating(int routing_id,
    peer::oipf::ParentalRatingItem* parental_rating) {
  proxy_->AddParentalRating(routing_id, parental_rating);
}

// OIPFCapabilitiesHostMsg_Bind
void CbipPluginIPC::BindOipfCapabilities(int routing_id) {
  proxy_->BindOipfCapabilities(routing_id);
}
// OIPFCapabilitiesHostMsg_Unbind
void CbipPluginIPC::UnbindOipfCapabilities(int routing_id) {
  proxy_->UnbindOipfCapabilities(routing_id);
}

// OIPFCapabilitiesHostMsg_GetXmlCapabilities
void CbipPluginIPC::GetXmlCapabilities(int routing_id,
                                       std::string* xml_capabilities) {
  proxy_->GetXmlCapabilities(routing_id, xml_capabilities);
}

// OIPFCapabilitiesHostMsg_GetExtraSDVideoDecodes
void CbipPluginIPC::GetExtraSDVideoDecodes(int routing_id,
                                           int32_t* extra_sd_video_decodes) {
  proxy_->GetExtraSDVideoDecodes(routing_id, extra_sd_video_decodes);
}

// OIPFCapabilitiesHostMsg_GetExtraHDVideoDecodes
void CbipPluginIPC::GetExtraHDVideoDecodes(int routing_id,
                                           int32_t* extra_hd_video_decodes) {
  proxy_->GetExtraSDVideoDecodes(routing_id, extra_hd_video_decodes);
}

// OIPFCapabilitiesHostMsg_HasCapabilities
 bool CbipPluginIPC::HasCapabilities(int routing_id, 
                                     const std::string& profile_name) {
  return proxy_->HasCapabilities(routing_id, profile_name);
}

}  // namespace content
