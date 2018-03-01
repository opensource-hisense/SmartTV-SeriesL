#include "content/browser/cbip/cbip_task_runner.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/tracked_objects.h"
#include "content/public/browser/browser_thread.h"

#define X_LOG_V 0

namespace content {

#if 0
/* FIXME: wrap */
CbipTaskRunner::CbipTaskRunner(
    base::SequencedWorkerPool* sequenced_worker_pool,
    base::SingleThreadTaskRunner* delayed_task_task_runner)
    : sequenced_worker_pool_(sequenced_worker_pool),
      delayed_task_task_runner_(delayed_task_task_runner) {
  primary_sequence_token_ = sequenced_worker_pool_->GetSequenceToken();
}
#else
/* FIXME: wrap */
CbipTaskRunner::CbipTaskRunner()
    : sequenced_worker_pool_(BrowserThread::GetBlockingPool()),
      delayed_task_task_runner_(
          BrowserThread::GetMessageLoopProxyForThread(BrowserThread::IO).get()) {
  primary_sequence_token_ = sequenced_worker_pool_->GetSequenceToken();
}
#endif

CbipTaskRunner::~CbipTaskRunner() {
  // FIXME: this will never be called because the owner, CbipInternals,
  // will never be destructed - causing memory leak.
  {
    VLOG(X_LOG_V) << "ZZZZ: CbipTaskRunner::dtor:"
                  << " this=" << static_cast<void*>(this)
      ;
  }
}

bool CbipTaskRunner::PostDelayedTask(
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    base::TimeDelta delay) {
  // Note base::TaskRunner implements PostTask in terms of PostDelayedTask
  // with a delay of zero, we detect that usage and avoid the unecessary
  // trip thru the message loop.
  if (delay == base::TimeDelta()) {
    return sequenced_worker_pool_->PostSequencedWorkerTaskWithShutdownBehavior(
        primary_sequence_token_, from_here, task,
        base::SequencedWorkerPool::BLOCK_SHUTDOWN);
  }
  // Post a task to call this->PostTask() after the delay.
  return delayed_task_task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(base::IgnoreResult(&CbipTaskRunner::PostTask),
                 this, from_here, task),
      delay);
}

bool CbipTaskRunner::RunsTasksOnCurrentThread() const {
  return sequenced_worker_pool_->IsRunningSequenceOnCurrentThread(
      primary_sequence_token_);
}

}  // namespace content
