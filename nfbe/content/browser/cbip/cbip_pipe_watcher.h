#ifndef CONTENT_BROWSER_CBIP_CBIP_PIPE_WATCHER_H_
#define CONTENT_BROWSER_CBIP_CBIP_PIPE_WATCHER_H_

#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"

namespace content {

class CbipInternals;

class CbipPipeWatcher
    : public base::MessageLoopForIO::Watcher {
 public:
  // FIXME: non-linux version.  factory.
  CbipPipeWatcher(CbipInternals* internals);
  ~CbipPipeWatcher() override;

  // OS_POSIX : base::MessageLoopForIO::Watcher == base::MessagePumpLibevent::Watcher
  void OnFileCanReadWithoutBlocking(int fd) override;
  void OnFileCanWriteWithoutBlocking(int fd) override;

 private:
  void init();  // on file thread, blocking
  void start(int fd);  // on IO thread, non-blocking

  CbipInternals* internals_;

  // FIXME: non-linux version.
  // the following members should only be accessed on the file thread.
  int pipe_fd_r_;
  int pipe_fd_w_;
  int retry_count_;

  // the following members should only be accessed on the IO thread.
  base::MessagePumpLibevent::FileDescriptorWatcher controller_;

  DISALLOW_COPY_AND_ASSIGN(CbipPipeWatcher);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CBIP_CBIP_PIPE_WATCHER_H_
