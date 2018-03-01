#include "content/browser/cbip/cbip_js00_message_filter.h"

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/sequenced_worker_pool.h"
#include "content/browser/bad_message.h"
#include "content/browser/cbip/cbip_internals.h"
#include "content/browser/cbip/cbip_task_runner.h"
#include "content/common/cbip/cbip_js00_messages.h"

#define X_LOG_V 0

#define X_HAS_TASK_RUNNER 0

namespace content {

CbipJs00MessageFilter::CbipJs00MessageFilter(
    int render_process_id,
    CbipTaskRunner* task_runner)
    : BrowserMessageFilter(CbipJs00MsgStart),
      render_process_id_(render_process_id),
      task_runner_(task_runner) {

  {
    VLOG(X_LOG_V) << "CbipJs00MessageFilter::ctor:"
                  << " this=" << this
                  << " render_process_id=" << render_process_id_
      ;
  }

  CbipInternals* internals = CbipInternals::GetInstance();
  internals->RegisterJs00MessageFilter(render_process_id_, this);

  /* FIXME: IMPL */
}

CbipJs00MessageFilter::~CbipJs00MessageFilter() {

  {
    VLOG(X_LOG_V) << "CbipJs00MessageFilter::dtor:"
                  << " this=" << this
                  << " render_process_id=" << render_process_id_
      ;
  }

  CbipInternals* internals = CbipInternals::GetInstance();
  internals->UnregisterJs00MessageFilter(render_process_id_, this);

  /* FIXME: IMPL */
}

void CbipJs00MessageFilter::InitializeInSequence() {
  // this does blocking operations so must not be on IO thread.
  // see below.
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
}

void CbipJs00MessageFilter::UninitializeInSequence() {
  // this does blocking operations so must not be on IO thread.
  // see below.
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
}

void CbipJs00MessageFilter::OnFilterAdded(IPC::Sender* sender) {
  // base::TaskRunner::PostTask is a base::TaskRunner::PostDelayedTask
  // with a delay of zero.
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&CbipJs00MessageFilter::InitializeInSequence, this),
      base::TimeDelta());
}

void CbipJs00MessageFilter::OnFilterRemoved() {
  // base::TaskRunner::PostTask is a base::TaskRunner::PostDelayedTask
  // with a delay of zero.
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&CbipJs00MessageFilter::UninitializeInSequence, this),
      base::TimeDelta());
}

base::TaskRunner* CbipJs00MessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& message) {
  if (IPC_MESSAGE_CLASS(message) == CbipJs00MsgStart) {
    // the default is to use our task runner.
    // for certain non-blocking operations, use the IO runner.
    if (message.type() == CbipJs00HostMsg_PT1_Async::ID ||
        message.type() == CbipJs00HostMsg_PT1_Async_Routed::ID)
      return NULL;
    return task_runner_.get();
  }
  return NULL;
}

bool CbipJs00MessageFilter::OnMessageReceived(const IPC::Message& message) {
  if (IPC_MESSAGE_CLASS(message) != CbipJs00MsgStart)
    return false;
  DCHECK((message.type() == CbipJs00HostMsg_PT1_Async::ID ||
          message.type() == CbipJs00HostMsg_PT1_Async_Routed::ID)
         ? BrowserThread::CurrentlyOn(BrowserThread::IO)
         : !BrowserThread::CurrentlyOn(BrowserThread::IO));

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(CbipJs00MessageFilter, message)
    IPC_MESSAGE_HANDLER(CbipJs00HostMsg_PT1_Async, OnPT1_Async)
    IPC_MESSAGE_HANDLER(CbipJs00HostMsg_PT1_Sync, OnPT1_Sync)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void CbipJs00MessageFilter::OnPT1_Async(const CbipJs00Msg_PT1_Params& i0) {

  {
    VLOG(X_LOG_V) << "ZZZZ:  CbipJs00MessageFilter::OnPT1_Async:"
                  << " this=" << this
                  << " render_process_id=" << render_process_id_
                  << " i0.m0 = " << i0.m0_string;
      ;
  }
  /* this function does non-blocking ops */
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  Send(new CbipJs00Msg_AsyncOperationComplete(true));
}

void CbipJs00MessageFilter::OnPT1_Sync(const CbipJs00Msg_PT1_Params& i0,
    CbipJs00Msg_PT1_Params* o0) {

  {
    VLOG(X_LOG_V) << "ZZZZ:  CbipJs00MessageFilter::OnPT1_Sync:"
                  << " this=" << this
                  << " render_process_id=" << render_process_id_
                  << " i0.m0 = " << i0.m0_string;
      ;
  }
  /* this function does blocking ops */
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));

  o0->m0_string = base::NullableString16(
      base::ASCIIToUTF16("dummy value from the browser"), false);
}

void CbipJs00MessageFilter::SendCbipJs00AsyncEvent1(
    const base::NullableString16& p0) {
  {
    VLOG(X_LOG_V) << "ZZZZ:  CbipJs00MessageFilter::SendCbipJs00AsyncEvent1:"
                  << " p0 = " << p0
      ;
  }
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&CbipJs00MessageFilter::InternalSendCbipJs00AsyncEvent1, this, p0),
      base::TimeDelta());
}

void CbipJs00MessageFilter::InternalSendCbipJs00AsyncEvent1(
    const base::NullableString16 p0) {
  {
    VLOG(X_LOG_V) << "ZZZZ:  CbipJs00MessageFilter::InternalSendCbipJs00AsyncEvent1:"
                  << " p0 = " << p0
      ;
  }
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  CbipJs00Msg_PT1_Params params;
  params.m0_string = p0;
  Send(new CbipJs00Msg_AsyncEvent1(params));
}

// FIXME: this should be a hidden function.
// a corresponding public function should do PostTask to
// |task_runner_| with completion callback.
void CbipJs00MessageFilter::SendCbipJs00SyncEvent1(
    const base::NullableString16& p0,
    base::NullableString16* o0,
    const CompletionCallback* callback) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  CbipJs00Msg_PT1_Params params;
  params.m0_string = p0;
  CbipJs00Msg_PT1_Params result;
  Send(new CbipJs00Msg_SyncEvent1(params, &result));
  /* FIXME: modify |o0| from |result|. */
  if (callback)
    callback->Run(true);
}

}  // namespace content
