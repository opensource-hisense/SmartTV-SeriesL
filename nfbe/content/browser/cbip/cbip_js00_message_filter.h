#ifndef CONTENT_BROWSER_CBIP_CBIP_JS00_MESSAGE_FILTER_H_
#define CONTENT_BROWSER_CBIP_CBIP_JS00_MESSAGE_FILTER_H_

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/browser_message_filter.h"

struct CbipJs00Msg_PT1_Params;

namespace base {
class NullableString16;
}

namespace content {

class CbipTaskRunner;  /* FIXME: wrap */

class CbipJs00MessageFilter
    : public BrowserMessageFilter {
 public:
  // callback for a xcall from the browser to a renderer.
  typedef base::Callback<void(bool)> CompletionCallback;

  explicit CbipJs00MessageFilter(int render_process_id,
                                 CbipTaskRunner* task_runner);

  void SendCbipJs00AsyncEvent1(
      const base::NullableString16& p0);

 private:
  ~CbipJs00MessageFilter() override;

  void InitializeInSequence();
  void UninitializeInSequence();

  // BrowserMessageFilter implementation
  void OnFilterAdded(IPC::Sender* sender) override;
  void OnFilterRemoved() override;
  base::TaskRunner* OverrideTaskRunnerForMessage(
      const IPC::Message& message) override;
  bool OnMessageReceived(const IPC::Message& message) override;

  // Message Handlers.
  void OnPT1_Async(const CbipJs00Msg_PT1_Params& params);
  void OnPT1_Sync(const CbipJs00Msg_PT1_Params& i0,
                  CbipJs00Msg_PT1_Params* o0);

  void InternalSendCbipJs00AsyncEvent1(
      const base::NullableString16 p0);

  // FIXME: impl.
  // someone must do PostTask SendCbipJs00MessageEvent
  // on a TaskRunner which can block, with an optional
  // callback.
  void SendCbipJs00SyncEvent1(
      const base::NullableString16& p0,
      base::NullableString16* o0,
      const CompletionCallback* callback);

  int render_process_id_;
  scoped_refptr<CbipTaskRunner> task_runner_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(CbipJs00MessageFilter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CBIP_CBIP_JS00_MESSAGE_FILTER_H_
