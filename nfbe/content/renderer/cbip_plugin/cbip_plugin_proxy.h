// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_PROXY_H_
#define CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_PROXY_H_

#include <string>
#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/string16.h"

namespace peer {
namespace oipf {
struct VideoBroadcastChannel;
}
}

namespace content {

class CbipPluginProxy : public base::RefCounted<CbipPluginProxy> {
 public:
  typedef base::Callback<void(bool)> CompletionCallback;

  virtual void PT1Async(const base::string16& i0,
                        const CompletionCallback& callback) = 0;

  virtual base::NullableString16 PT1Sync(const base::string16& i0) = 0;

  virtual void PT1AsyncRouted(int routing_id, const base::string16& i0)
      = 0;

  virtual uint32_t GetOwnerApplication0(int routing_id) = 0;

  virtual uint32_t CreateApplication(int routing_id,
                                     uint32_t parent_oipf_application_index,
                                     const GURL& uri,
                                     bool createChild) = 0;

  virtual void DestroyApplication(int routing_id,
                                  uint32_t oipf_application_index) = 0;

  virtual void Show(int routing_id, uint32_t oipf_application_index) = 0;

  virtual void Hide(int routing_id, uint32_t oipf_application_index) = 0;

  virtual void ApplicationDeleted(int routing_id, uint32_t oipf_application_index) = 0;

  virtual double GetFreeMem(int routing_id) = 0;

  virtual void GetConfigurationData(int routing_id, ConfigurationParam *configuration_param) = 0;

  virtual void GetConfigurationLocalSystem(int routing_id,
                                           ConfigurationLocalSysParam* configuration_local_sys_param) = 0;

  virtual void Bind(int routing_id) = 0;
  virtual void Unbind(int routing_id) = 0;
  virtual void SetFullScreen(int routing_id, bool is_full_screen) = 0;
  virtual void GetPlayState(int routing_id, int* play_state) = 0;
  virtual void GetCurrentChannel(int routing_id, bool* is_success, peer::oipf::VideoBroadcastChannel* channel) = 0;
  virtual void SetVolume(int routing_id, int volume) = 0;
  virtual void GetVolume(int routing_id, int* volume) = 0;
  virtual void BindToCurrentChannel(int routing_id) = 0;
  virtual void CreateChannelObject(
      int routing_id, const VideoBroadcastCreateChannelObjectSiDirectParam& param,
      bool* is_success,
      peer::oipf::VideoBroadcastChannel* channel) = 0;
  virtual void CreateChannelObject(
      int routing_id, const VideoBroadcastCreateChannelObjectOtherParam& param,
      bool* is_success,
      peer::oipf::VideoBroadcastChannel* channel) = 0;
  virtual void SetChannel(int routing_id, const peer::oipf::VideoBroadcastChannel& channel, bool trickplay, const std::string& url) = 0;
  virtual void PrevChannel(int routing_id) = 0;
  virtual void NextChannel(int routing_id) = 0;
  virtual void VideoBroadcastRelease(int routing_id) = 0;
  virtual void Stop(int routing_id) = 0;
  virtual void GetComponents(int routing_id, int component_type, bool* is_success,
                             peer::oipf::AVComponentCollection* component_collection) = 0;
  virtual void GetCurrentActiveComponents(
      int routing_id,
      int component_type,
      bool* is_success,
      peer::oipf::AVComponentCollection* compoent_collection) = 0;
  virtual void SelectComponent(int routing_id, int component_type,
                               peer::oipf::AVComponentsData *av_components_data) = 0;
  virtual void UnselectComponent(int routing_id, int component_type,
                                 peer::oipf::AVComponentsData *av_components_data) = 0;

  virtual void DidChangeView(int routing_id, const gfx::Rect& rect) = 0;

  virtual int GetNumProgrammes(int routing_id) = 0;
  virtual void GetProgrammeItemByIndex(int routing_id, int index, bool* is_success, peer::oipf::VideoBroadcastProgramme* programme) = 0;
  virtual void GetProgrammeSIDescriptors(int routing_id, std::string programme_id_, 
      int descriptor_tag, int descriptor_tag_extension, int private_data_specifier, bool* is_success, 
      peer::oipf::StringCollectionData *programme_string_collection) = 0;
  
  virtual void BindParentalControlManager(int routing_id) = 0;
  virtual void UnbindParentalControlManager(int routing_id) = 0;
  virtual int GetNumOfParentralRatingSchemes(int routing_id) = 0;
  virtual void GetParentalRatingSchemeByIndex(int routing_id,
      uint32_t index,
      bool* is_success,
      peer::oipf::ParentalRatingScheme* parental_rating_scheme) = 0;
  virtual void GetParentalRatingSchemeByName(int routing_id,
      const std::string& name,
      bool* is_success,
      peer::oipf::ParentalRatingScheme* parental_rating_scheme) = 0;
  virtual void AddParentalRating(int routing_id,
      peer::oipf::ParentalRatingItem* parental_rating) = 0;

  virtual void BindOipfCapabilities(int routing_id) = 0;
  virtual void UnbindOipfCapabilities(int routing_id) = 0;
  virtual void GetXmlCapabilities(int routing_id,
                                  std::string* xml_capabilities) = 0;
  virtual void GetExtraSDVideoDecodes(int routing_id,
                                      int32_t* extra_sd_video_decodes) = 0;
  virtual void GetExtraHDVideoDecodes(int routing_id,
                                      int32_t* extra_hd_video_decodes) = 0;
  virtual bool HasCapabilities(int routing_id,
                               const std::string& profile_name) = 0;
 protected:
  friend class base::RefCounted<CbipPluginProxy>;
  virtual ~CbipPluginProxy() {}
};

}  // namespace content

#endif  // CONTENT_RENDERER_CBIP_PLUGIN_CBIP_PLUGIN_PROXY_H_
