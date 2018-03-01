#ifndef CONTENT_BROWSER_CBIP_CBIP_TASK_RUNNER_H_
#define CONTENT_BROWSER_CBIP_CBIP_TASK_RUNNER_H_

#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/time/time.h"
#include "content/common/content_export.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace content {

// you can create multiple workers.  see dom_storage_task_runner.

class CONTENT_EXPORT CbipTaskRunner
    : public base::TaskRunner {
 public:
#if 0
  /* FIXME: wrap */
  CbipTaskRunner(
      base::SequencedWorkerPool* sequenced_worker_pool,
      base::SingleThreadTaskRunner* delayed_task_task_runner);
#else
  /* FIXME: wrap */
  CbipTaskRunner();
#endif

  // The PostTask() and PostDelayedTask() methods defined by this TaskRunner
  // post shutdown-blocking tasks.
  bool PostDelayedTask(const tracked_objects::Location& from_here,
                       const base::Closure& task,
                       base::TimeDelta delay) override;

  bool RunsTasksOnCurrentThread() const override;

 protected:
  ~CbipTaskRunner() override;

 private:
  const scoped_refptr<base::SequencedWorkerPool> sequenced_worker_pool_;
  base::SequencedWorkerPool::SequenceToken primary_sequence_token_;
  const scoped_refptr<base::SingleThreadTaskRunner> delayed_task_task_runner_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_CBIP_CBIP_TASK_RUNNER_H_
