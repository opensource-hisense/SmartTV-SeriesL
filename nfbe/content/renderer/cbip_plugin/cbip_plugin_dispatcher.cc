// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.
#include "content/renderer/cbip_plugin/cbip_plugin_dispatcher.h"

#include <list>
#include <map>
#include <string>

#include "access/content/browser/cbip_plugin_host/oipf_message_handler.h"
#include "access/content/common/cbip/oipf_messages.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "content/common/cbip/cbip_js00_messages.h"
#include "content/common/cbip/hbbtv_messages.h"
#include "content/renderer/cbip_plugin/cbip_plugin_ipc.h"
#include "content/renderer/cbip_plugin/cbip_plugin_object.h"
#include "content/renderer/cbip_plugin/cbip_plugin_proxy.h"
#include "content/renderer/render_thread_impl.h"
#include "ipc/message_filter.h"
#include "third_party/WebKit/public/platform/Platform.h"
#include "third_party/WebKit/public/web/WebKit.h"

#define X_LOG_V 0

namespace content {

namespace {
// MessageThrottlingFilter -------------------------------------------
// Used to limit the number of ipc messages pending completion so we
// don't overwhelm the main browser process. When the limit is reached,
// a synchronous message is sent to flush all pending messages thru.
// We expect to receive an 'ack' for each message sent. This object
// observes receipt of the acks on the IPC thread to decrement a counter.
class MessageThrottlingFilter : public IPC::MessageFilter {
 public:
  explicit MessageThrottlingFilter(RenderThreadImpl* sender)
      : pending_count_(0), sender_(sender) {}

  void SendThrottled(IPC::Message* message);
  void Shutdown() { sender_ = NULL; }

 private:
  ~MessageThrottlingFilter() override {}

  bool OnMessageReceived(const IPC::Message& message) override;

  int GetPendingCount() { return IncrementPendingCountN(0); }
  int IncrementPendingCount() { return IncrementPendingCountN(1); }
  int DecrementPendingCount() { return IncrementPendingCountN(-1); }
  int IncrementPendingCountN(int increment) {
    base::AutoLock locker(lock_);
    pending_count_ += increment;
    return pending_count_;
  }

  base::Lock lock_;
  int pending_count_;
  RenderThreadImpl* sender_;
};

void MessageThrottlingFilter::SendThrottled(IPC::Message* message) {
  // Should only be used for sending of messages which will be acknowledged
  // with a separate CbipJs00Msg_AsyncOperationComplete message.
  DCHECK(message->type() == CbipJs00HostMsg_PT1_Async::ID);
  DCHECK(sender_);
  if (!sender_) {
    delete message;
    return;
  }
  const int kMaxPendingMessages = 1000;
  bool need_to_flush = (IncrementPendingCount() > kMaxPendingMessages) &&
                       !message->is_sync();
  sender_->Send(message);
  if (need_to_flush) {
    sender_->Send(new CbipJs00HostMsg_FlushMessages);
    DCHECK_EQ(0, GetPendingCount());
  } else {
    DCHECK_LE(0, GetPendingCount());
  }
}

bool MessageThrottlingFilter::OnMessageReceived(const IPC::Message& message) {
  if (message.type() == CbipJs00Msg_AsyncOperationComplete::ID) {

    {
      VLOG(X_LOG_V) << "MessageThrottlingFilter::OnMessageReceived:"
                    << " type == CbipJs00Msg_AsyncOperationComplete::ID"
        ;
    }

    DecrementPendingCount();
    DCHECK_LE(0, GetPendingCount());
  }
  return false;
}
}  // namespace


// ProxyImpl -----------------------------------------------------
// An implementation of the CbipPluginProxy interface in terms of IPC.
// This class also manages the application manager and pending
// operations awaiting completion callbacks.
class CbipPluginDispatcher::ProxyImpl : public CbipPluginProxy {
 public:
  explicit ProxyImpl(RenderThreadImpl* sender);

  // Methods for use by CbipPluginDispatcher directly.
  CbipPluginIPC* GetAppMgr(int cbip_plugin_instance_id);
  void ReleaseAppMgr(int cbip_plugin_instance_id, CbipPluginIPC* appmgr);
  void CompleteOnePendingCallback(bool success);
  void Shutdown();
  bool OnAppMgrMessageReceived(const IPC::Message& msg);

  // CbipPluginProxy interface for use by CbipPluginIPC.
  void PT1Async(const base::string16& i0,
                const CompletionCallback& callback) override;
  base::NullableString16 PT1Sync(const base::string16& i0) override;
  void PT1AsyncRouted(int routing_id, const base::string16& i0)
      override;
  uint32_t GetOwnerApplication0(int routing_id) override;
  uint32_t CreateApplication(int routing_id,
                             uint32_t parent_oipf_application_index,
                             const GURL& uri,
                             bool createChild) override;
  void DestroyApplication(int routing_id, uint32_t oipf_application_index)
      override;
  void Show(int routing_id, uint32_t oipf_application_index) override;
  void Hide(int routing_id, uint32_t oipf_application_index) override;
  void ApplicationDeleted(int routing_id, uint32_t oipf_application_index) override;
  double GetFreeMem(int routing_id) override;

  void GetConfigurationData(int routing_id, ConfigurationParam* configuration_param) override;
  void GetConfigurationLocalSystem(int routing_id, ConfigurationLocalSysParam* configuration_local_sys_param) override;
  // video/broadcast plug-in
  void Bind(int routing_id) override;
  void Unbind(int routing_id) override;
  void SetFullScreen(int routing_id, bool is_full_screen) override;
  void GetPlayState(int routing_id, int* play_state) override;
  void GetCurrentChannel(int routing_id, bool* is_success, peer::oipf::VideoBroadcastChannel* channel) override;
  void SetVolume(int routing_id, int volume) override;
  void GetVolume(int routing_id, int* volume) override;
  void BindToCurrentChannel(int routing_id) override;
  void CreateChannelObject(int routing_id,
                           const VideoBroadcastCreateChannelObjectSiDirectParam& param,
                           bool* is_success,
                           peer::oipf::VideoBroadcastChannel* channel) override;
  void CreateChannelObject(int routing_id,
                           const VideoBroadcastCreateChannelObjectOtherParam& param,
                           bool* is_success,
                           peer::oipf::VideoBroadcastChannel* channel) override;
  void SetChannel(int routing_id, const peer::oipf::VideoBroadcastChannel& channel, bool trickplay, const std::string& url) override;
  void PrevChannel(int routing_id) override;
  void NextChannel(int routing_id) override;
  void VideoBroadcastRelease(int routing_id) override;
  void Stop(int routing_id) override;
  void GetComponents(int routing_id,
                     int component_type,
                     bool* is_success,
                     peer::oipf::AVComponentCollection* component_collection) override;
  void GetCurrentActiveComponents(int routing_id, int component_type,
                                  bool* is_success,
                                  peer::oipf::AVComponentCollection* component_collection) override;
  void SelectComponent(int routing_id, int component_type,
                       peer::oipf::AVComponentsData* av_components_data) override;
  void UnselectComponent(int routing_id, int component_type,
                         peer::oipf::AVComponentsData* av_components_data) override;

  void DidChangeView(int routing_id, const gfx::Rect& rect) override;

  int GetNumProgrammes(int routing_id) override;
  void GetProgrammeItemByIndex(int routing_id, int index, bool* is_success, peer::oipf::VideoBroadcastProgramme* programme) override;
  void GetProgrammeSIDescriptors(int routing_id, std::string programme_id, int descriptor_tag,
    int descriptor_tag_extension, int private_data_specifier, bool* is_success,
    peer::oipf::StringCollectionData* programme_string_collection) override;

  // application/ParentalControlManager plug-in
  void BindParentalControlManager(int routing_id) override;
  void UnbindParentalControlManager(int routing_id) override;
  int GetNumOfParentralRatingSchemes(int routing_id) override;
  void GetParentalRatingSchemeByIndex(
      int routing_id,
      uint32_t index,
      bool* is_success,
      peer::oipf::ParentalRatingScheme* parental_rating_scheme) override;
  void GetParentalRatingSchemeByName(int routing_id,
      const std::string& name,
      bool* is_success,
      peer::oipf::ParentalRatingScheme* parental_rating_scheme) override;
  void AddParentalRating(int routing_id,
      peer::oipf::ParentalRatingItem* parental_rating) override;

  void BindOipfCapabilities(int routing_id) override;
  void UnbindOipfCapabilities(int routing_id) override;
  void GetXmlCapabilities(int routing_id,
                          std::string* xml_capabilities) override;
  void GetExtraSDVideoDecodes(int routing_id,
                              int32_t* extra_sd_video_decodes) override;
  void GetExtraHDVideoDecodes(int routing_id,
                              int32_t* extra_hd_video_decodes) override;
  bool HasCapabilities(int routing_id,
                       const std::string& profile_name) override;

 private:
  /* FIXME: currently, CbipPluginIPC and CbipPluginInstance are
   * one to one correspondence, but in reality, they are not.
   * so there must be IPC list, and each IPC has PluginInstance list.
   */
  typedef std::list<int> CbipPluginInstanceList;
  typedef std::list<CompletionCallback> CallbackList;

  ~ProxyImpl() override {}

  // Sudden termination is disabled when there are callbacks pending
  // to more reliably commit changes during shutdown.
  void PushPendingCallback(const CompletionCallback& callback) {
    if (pending_callbacks_.empty())
      blink::Platform::current()->suddenTerminationChanged(false);
    pending_callbacks_.push_back(callback);
  }

  CompletionCallback PopPendingCallback() {
    CompletionCallback callback = pending_callbacks_.front();
    pending_callbacks_.pop_front();
    if (pending_callbacks_.empty())
      blink::Platform::current()->suddenTerminationChanged(true);
    return callback;
  }

  RenderThreadImpl* sender_;
  CbipPluginInstanceList cbip_instances_;
  CallbackList pending_callbacks_;
  scoped_refptr<MessageThrottlingFilter> throttling_filter_;
};

CbipPluginDispatcher::ProxyImpl::ProxyImpl(RenderThreadImpl* sender)
    : sender_(sender),
      throttling_filter_(new MessageThrottlingFilter(sender)) {
  sender_->AddFilter(throttling_filter_.get());
}

CbipPluginIPC* CbipPluginDispatcher::ProxyImpl::GetAppMgr(
    int cbip_plugin_instance_id) {
#if 0
  scoped_refptr<CbipPluginIPC> appmgr = new CbipPluginIPC(this);
  _register_appmgr_to_someone_which_increments_refcount_of_appmgr_(appmgr.get());
  return appmgr.get();
#else
  DCHECK(std::find(cbip_instances_.begin(), cbip_instances_.end(),
                   cbip_plugin_instance_id) == cbip_instances_.end());
  CbipPluginIPC* appmgr_ptr = new CbipPluginIPC(this);
  cbip_instances_.push_back(cbip_plugin_instance_id);
  return appmgr_ptr;
#endif
}

void CbipPluginDispatcher::ProxyImpl::ReleaseAppMgr(int cbip_plugin_instance_id,
    CbipPluginIPC* appmgr) {
#if 0
  _unregister_appmgr_to_decrement_refcount_so_that_appmgr_will_be_freed_later(appmgr);
#else
  DCHECK(std::find(cbip_instances_.begin(), cbip_instances_.end(),
                   cbip_plugin_instance_id) != cbip_instances_.end());
  cbip_instances_.erase(std::find(cbip_instances_.begin(), cbip_instances_.end(),
      cbip_plugin_instance_id));
#endif
}

void CbipPluginDispatcher::ProxyImpl::CompleteOnePendingCallback(bool success) {
  PopPendingCallback().Run(success);
}

void CbipPluginDispatcher::ProxyImpl::Shutdown() {
  throttling_filter_->Shutdown();
  sender_->RemoveFilter(throttling_filter_.get());
  sender_ = NULL;
  pending_callbacks_.clear();
}

void CbipPluginDispatcher::ProxyImpl::PT1Async(
    const base::string16& i0, const CompletionCallback& callback) {

  /* CbipJs00HostMsg_PT1_Async messsage needs an ack. */
  PushPendingCallback(callback);

  CbipJs00Msg_PT1_Params params;
  /* FIXME: should we convert empty string to null ? ? ? */
  params.m0_string = base::NullableString16(i0, false);

  /* CbipJs00HostMsg_PT1_Async messsage needs an ack. */
  throttling_filter_->SendThrottled(new CbipJs00HostMsg_PT1_Async(params));
}

base::NullableString16 CbipPluginDispatcher::ProxyImpl::PT1Sync(
    const base::string16& i0) {

  /* CbipJs00HostMsg_PT1_Sync messsage does not need an ack. */

  CbipJs00Msg_PT1_Params params_i0;
  CbipJs00Msg_PT1_Params params_o0;
  /* FIXME: should we convert empty string to null ? ? ? */
  params_i0.m0_string = base::NullableString16(i0, false);

  /* CbipJs00HostMsg_PT1_Sync messsage does not need an ack. */
  sender_->Send(new CbipJs00HostMsg_PT1_Sync(params_i0, &params_o0));

  return params_o0.m0_string;
}

void CbipPluginDispatcher::ProxyImpl::PT1AsyncRouted(
    int routing_id,
    const base::string16& i0) {

  /* CbipJs00HostMsg_PT1_Async_Routed messsage does not need an ack. */

  CbipJs00Msg_PT1_Params params;
  params.m0_string = base::NullableString16(i0, false);

  sender_->Send(new CbipJs00HostMsg_PT1_Async_Routed(routing_id, params));
}

uint32_t CbipPluginDispatcher::ProxyImpl::GetOwnerApplication0(int routing_id) {
  uint32_t opif_app_index = 0;
  sender_->Send(new HbbtvHostMsg_GET_OWNER_APPLICATION_0(routing_id, &opif_app_index));
  return opif_app_index;
}

uint32_t CbipPluginDispatcher::ProxyImpl::CreateApplication(int routing_id,
    uint32_t parent_oipf_application_index,
    const GURL& uri,
    bool createChild) {
  uint32_t opif_app_index = 0;
  sender_->Send(new HbbtvHostMsg_CREATE_APP(routing_id,
      parent_oipf_application_index, uri, createChild, &opif_app_index));
  return opif_app_index;
}

void CbipPluginDispatcher::ProxyImpl::DestroyApplication(int routing_id,
    uint32_t oipf_application_index) {
  sender_->Send(new HbbtvHostMsg_DESTROY_APP(routing_id, oipf_application_index));
}

void CbipPluginDispatcher::ProxyImpl::Show(int routing_id,
    uint32_t oipf_application_index) {
  sender_->Send(new HbbtvHostMsg_SHOW_APP(routing_id, oipf_application_index));
}

void CbipPluginDispatcher::ProxyImpl::Hide(int routing_id,
    uint32_t oipf_application_index) {
  sender_->Send(new HbbtvHostMsg_HIDE_APP(routing_id, oipf_application_index));
}

void CbipPluginDispatcher::ProxyImpl::ApplicationDeleted(int routing_id,
    uint32_t oipf_application_index) {
  sender_->Send(new HbbtvHostMsg_APP_DELETED(routing_id, oipf_application_index));
}

double CbipPluginDispatcher::ProxyImpl::GetFreeMem(int routing_id) {
  double value = -1;
  sender_->Send(new HbbtvHostMsg_GET_FREE_MEM(routing_id, &value));
  return value;
}

void CbipPluginDispatcher::ProxyImpl::GetConfigurationData(int routing_id, ConfigurationParam* configuration_param) {
  sender_->Send(new ConfigurationHostMsg_FillConfigurationParam(routing_id, configuration_param));
}

void CbipPluginDispatcher::ProxyImpl::GetConfigurationLocalSystem(int routing_id, ConfigurationLocalSysParam* configuration_local_sys_param) {
  sender_->Send(new ConfigurationHostMsg_FillConfigurationLocalSystemParam(routing_id, configuration_local_sys_param));
}

void CbipPluginDispatcher::ProxyImpl::Bind(int routing_id) {
  sender_->Send(new VideoBroadcastHostMsg_Bind(routing_id));
}

void CbipPluginDispatcher::ProxyImpl::Unbind(int routing_id) {
  sender_->Send(new VideoBroadcastHostMsg_Unbind(routing_id));
}

void CbipPluginDispatcher::ProxyImpl::SetFullScreen(int routing_id, bool is_full_screen) {
  sender_->Send(new VideoBroadcastHostMsg_SetFullScreen(routing_id, is_full_screen));
}

void CbipPluginDispatcher::ProxyImpl::GetPlayState(int routing_id, int* play_state) {
  sender_->Send(new VideoBroadcastHostMsg_GetPlayState(routing_id, play_state));
}

void CbipPluginDispatcher::ProxyImpl::GetCurrentChannel(int routing_id, bool* is_success, peer::oipf::VideoBroadcastChannel* channel) {
  sender_->Send(new VideoBroadcastHostMsg_GetCurrentChannel(routing_id, is_success, channel));
}

void CbipPluginDispatcher::ProxyImpl::SetVolume(int routing_id, int volume) {
  sender_->Send(new VideoBroadcastHostMsg_SetVolume(routing_id, volume));
}

void CbipPluginDispatcher::ProxyImpl::GetVolume(int routing_id, int* volume) {
  sender_->Send(new VideoBroadcastHostMsg_GetVolume(routing_id, volume));
}

void CbipPluginDispatcher::ProxyImpl::BindToCurrentChannel(int routing_id) {
  sender_->Send(new VideoBroadcastHostMsg_BindToCurrentChannel(routing_id));
}

void CbipPluginDispatcher::ProxyImpl::CreateChannelObject(
    int routing_id,
    const VideoBroadcastCreateChannelObjectSiDirectParam& param,
    bool* is_success,
    peer::oipf::VideoBroadcastChannel* channel) {
  sender_->Send(new VideoBroadcastHostMsg_CreateChannelObjectSiDirect(
      routing_id, param, is_success, channel));
}

void CbipPluginDispatcher::ProxyImpl::CreateChannelObject(
    int routing_id,
    const VideoBroadcastCreateChannelObjectOtherParam& param,
    bool* is_success,
    peer::oipf::VideoBroadcastChannel* channel) {
  sender_->Send(new VideoBroadcastHostMsg_CreateChannelObjectOther(
      routing_id, param, is_success, channel));
}

void CbipPluginDispatcher::ProxyImpl::SetChannel(int routing_id, const peer::oipf::VideoBroadcastChannel& channel, bool trickplay, const std::string& url) {
  sender_->Send(new VideoBroadcastHostMsg_SetChannel(routing_id, channel, trickplay, url));
}

void CbipPluginDispatcher::ProxyImpl::PrevChannel(int routing_id) {
  sender_->Send(new VideoBroadcastHostMsg_PrevChannel(routing_id));
}

void CbipPluginDispatcher::ProxyImpl::NextChannel(int routing_id) {
  sender_->Send(new VideoBroadcastHostMsg_NextChannel(routing_id));
}

void CbipPluginDispatcher::ProxyImpl::VideoBroadcastRelease(int routing_id) {
  sender_->Send(new VideoBroadcastHostMsg_Release(routing_id));
}

void CbipPluginDispatcher::ProxyImpl::Stop(int routing_id) {
  sender_->Send(new VideoBroadcastHostMsg_Stop(routing_id));
}

void CbipPluginDispatcher::ProxyImpl::GetComponents(
    int routing_id,
    int component_type,
    bool* is_success,
    peer::oipf::AVComponentCollection* component_collection) {
  sender_->Send(new VideoBroadcastHostMsg_GetComponents(routing_id, component_type,
                is_success, component_collection));
}

void CbipPluginDispatcher::ProxyImpl::GetCurrentActiveComponents(
    int routing_id,
    int component_type,
    bool* is_success,
    peer::oipf::AVComponentCollection* component_collection) {
  sender_->Send(new VideoBroadcastHostMsg_GetCurrentActiveComponents(routing_id, component_type,
                is_success, component_collection));
}

void CbipPluginDispatcher::ProxyImpl::SelectComponent(
    int routing_id,
    int component_type,
    peer::oipf::AVComponentsData* av_components_data) {
  sender_->Send(new VideoBroadcastHostMsg_SelectComponent(routing_id, component_type,
                *av_components_data));
}

void CbipPluginDispatcher::ProxyImpl::UnselectComponent(
    int routing_id,
    int component_type,
    peer::oipf::AVComponentsData* av_components_data) {
  sender_->Send(new VideoBroadcastHostMsg_UnselectComponent(routing_id, component_type,
                *av_components_data));
}

void CbipPluginDispatcher::ProxyImpl::DidChangeView(int routing_id, const gfx::Rect& rect) {
  sender_->Send(new VideoBroadcastHostMsg_DidChangeView(routing_id, rect));
}

int CbipPluginDispatcher::ProxyImpl::GetNumProgrammes(int routing_id) {
  int number_programmes = 0;
  sender_->Send(new VideoBroadcastHostMsg_GetNumProgrammes(routing_id, &number_programmes));
  return number_programmes;
}

void CbipPluginDispatcher::ProxyImpl::GetProgrammeItemByIndex(int routing_id, int index, bool* is_success, peer::oipf::VideoBroadcastProgramme* programme) {
  CHECK(is_success);
  CHECK(programme);
  sender_->Send(new VideoBroadcastHostMsg_GetProgrammeItemByIndex(routing_id, index, is_success, programme));
}

void CbipPluginDispatcher::ProxyImpl::GetProgrammeSIDescriptors(int routing_id, std::string programme_id,
    int descriptor_tag, int descriptor_tag_extension, int private_data_specifier, bool* is_success,
    peer::oipf::StringCollectionData* programme_string_collection) {
  sender_->Send(new VideoBroadcastHostMsg_GetProgrammeSIDescriptors(routing_id, programme_id, descriptor_tag,
               descriptor_tag_extension, private_data_specifier, is_success, programme_string_collection));
}

bool CbipPluginDispatcher::ProxyImpl::OnAppMgrMessageReceived(
    const IPC::Message& msg)
{
  {
    VLOG(X_LOG_V) << "ZZZZ: CbipPluginDispatcher::ProxyImpl::OnAppMgrMessageReceived"
      ;
  }

  for (CbipPluginInstanceList::iterator i = cbip_instances_.begin();
       i != cbip_instances_.end(); ++i) {
    CbipPluginObject* o = CbipPluginObject::FromInstanceId(*i);

    {
      VLOG(X_LOG_V) << "ZZZZ: CbipPluginObject "
                    << *i << "=" << static_cast<void*>(o)
        ;
    }

    if (o) {
      o->OnAppMgrMessageReceived(msg);
    }
  }

  return false;
}

// application/ParentalControlManager plug-in

void CbipPluginDispatcher::ProxyImpl::BindParentalControlManager(
    int routing_id) {
  sender_->Send(new ParentalControlManagerHostMsg_Bind(routing_id));
}

void CbipPluginDispatcher::ProxyImpl::UnbindParentalControlManager(
    int routing_id) {
  sender_->Send(new ParentalControlManagerHostMsg_Unbind(routing_id));
}

int CbipPluginDispatcher::ProxyImpl::GetNumOfParentralRatingSchemes(
    int routing_id) {
  int number_of_schemes = 0;
  sender_->Send(
      new ParentalControlManagerHostMsg_GetNumberOfParentalRatingSchemes(
          routing_id,
          &number_of_schemes));
  return number_of_schemes;
}
void CbipPluginDispatcher::ProxyImpl::GetParentalRatingSchemeByIndex(
    int routing_id,
    uint32_t index,
    bool* is_success,
    peer::oipf::ParentalRatingScheme* parental_rating_scheme) {
  sender_->Send(
      new ParentalControlManagerHostMsg_GetParentalRatingSchemeByIndex(
          routing_id,
          index,
          is_success,
          parental_rating_scheme));
}

void CbipPluginDispatcher::ProxyImpl::GetParentalRatingSchemeByName(
    int routing_id,
    const std::string& name,
    bool* is_success,
    peer::oipf::ParentalRatingScheme* parental_rating_scheme) {
  sender_->Send(
      new ParentalControlManagerHostMsg_GetParentalRatingSchemeByName(
          routing_id,
          name,
          is_success,
          parental_rating_scheme));
}
void CbipPluginDispatcher::ProxyImpl::AddParentalRating(int routing_id,
    peer::oipf::ParentalRatingItem* parental_rating) {
  sender_->Send(
      new ParentalRatingCollectionHostMsg_AddParentalRating(
         routing_id, *parental_rating));
}

void CbipPluginDispatcher::ProxyImpl::BindOipfCapabilities(int routing_id) {
  sender_->Send(new OIPFCapabilitiesHostMsg_Bind(routing_id));
}

void CbipPluginDispatcher::ProxyImpl::UnbindOipfCapabilities(int routing_id) {
  sender_->Send(new OIPFCapabilitiesHostMsg_Unbind(routing_id));
}

void CbipPluginDispatcher::ProxyImpl::GetXmlCapabilities(
    int routing_id,
    std::string* xml_capabilities) {
  sender_->Send(
      new OIPFCapabilitiesHostMsg_GetXmlCapabilities(
          routing_id,
          xml_capabilities));
}

void CbipPluginDispatcher::ProxyImpl::GetExtraSDVideoDecodes(
    int routing_id,
    int32_t* extra_sd_video_decodes) {
  sender_->Send(
      new OIPFCapabilitiesHostMsg_GetExtraSDVideoDecodes(
          routing_id,
          extra_sd_video_decodes));
}

void CbipPluginDispatcher::ProxyImpl::GetExtraHDVideoDecodes(
    int routing_id,
    int32_t* extra_hd_video_decodes) {
  sender_->Send(
      new OIPFCapabilitiesHostMsg_GetExtraSDVideoDecodes(
          routing_id,
          extra_hd_video_decodes));
}

bool CbipPluginDispatcher::ProxyImpl::HasCapabilities(
    int routing_id,
    const std::string& profile_name) {
  bool has_capability = false;
  sender_->Send(
      new OIPFCapabilitiesHostMsg_HasCapabilities(
          routing_id,
          profile_name,
          &has_capability));
  return has_capability;
}

// CbipPluginDispatcher ------------------------------------------------

CbipPluginDispatcher::CbipPluginDispatcher()
    : proxy_(new ProxyImpl(RenderThreadImpl::current())) {
}

CbipPluginDispatcher::~CbipPluginDispatcher() {
  proxy_->Shutdown();
}

scoped_refptr<CbipPluginIPC> CbipPluginDispatcher::GetAppMgr(
    int cbip_plugin_instance_id) {
#if 0
  /* FIXME: impl: do something to obtain the appmgr */
  RenderThreadImpl::current()->Send(
      new CbipHostMsg_GetAppMgr());
#endif
  return proxy_->GetAppMgr(cbip_plugin_instance_id);
}

void CbipPluginDispatcher::ReleaseAppMgr(int cbip_plugin_instance_id,
    CbipPluginIPC* appmgr) {
#if 0
  /* FIXME: impl: do something to tell the appmgr that the app has gone. */
  RenderThreadImpl::current()->Send(
      new CbipHostMsg_ReleaseAppMgr());
#endif
  proxy_->ReleaseAppMgr(cbip_plugin_instance_id, appmgr);
}

bool CbipPluginDispatcher::OnMessageReceived(const IPC::Message& msg) {
  if (IPC_MESSAGE_CLASS(msg) != CbipJs00MsgStart)
    return false;
  if (msg.type() == CbipJs00Msg_AsyncEvent1::ID ||
      msg.type() == CbipJs00Msg_SyncEvent1::ID) {
    if (proxy_->OnAppMgrMessageReceived(msg))
      return true;
  }
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(CbipPluginDispatcher, msg)
    IPC_MESSAGE_HANDLER(CbipJs00Msg_AsyncEvent1, OnAsyncEvent1)
    IPC_MESSAGE_HANDLER(CbipJs00Msg_SyncEvent1, OnSyncEvent1)
    IPC_MESSAGE_HANDLER(CbipJs00Msg_AsyncOperationComplete,
                        OnAsyncOperationComplete)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

// this is for sync message replies to the browser only.
// FIXME: friend?
void CbipPluginDispatcher::Send(IPC::Message* message) {
  RenderThreadImpl::current()->Send(message);
}

void CbipPluginDispatcher::OnAsyncEvent1(
    const CbipJs00Msg_PT1_Params& params) {
  {
    VLOG(X_LOG_V) << "ZZZZ: CbipPluginDispatcher::OnAsyncEvent1:"
                  << " params.m0_string = " << params.m0_string
      ;
  }
}

void CbipPluginDispatcher::OnSyncEvent1(
    const CbipJs00Msg_PT1_Params& params,
    CbipJs00Msg_PT1_Params *o0) {
  {
    VLOG(X_LOG_V) << "ZZZZ: CbipPluginDispatcher::OnSyncEvent1:"
                  << " params.m0_string = " << params.m0_string
      ;
  }
}

void CbipPluginDispatcher::OnAsyncOperationComplete(bool success) {
  {
    VLOG(X_LOG_V) << "ZZZZ: CbipPluginDispatcher::OnAsyncOperationComplete:"
                  << " " << success
      ;
  }
  proxy_->CompleteOnePendingCallback(success);
}

}  // namespace content
