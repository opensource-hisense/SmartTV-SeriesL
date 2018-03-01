// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "low_memory_listener.h"

#include <fcntl.h>

#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/sys_info.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "low_memory_listener_delegate.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/zygote_host_linux.h"

using content::BrowserThread;

namespace content {

namespace {
// This is the minimum amount of time in milliseconds between checks for
// low memory.
const int kLowMemoryCheckTimeoutMs = 750;
}  // namespace

////////////////////////////////////////////////////////////////////////////////
// LowMemoryListenerImpl
//
// Does the actual work of observing.  The observation work happens on the FILE
// thread, and notification happens on the UI thread.  If low memory is
// detected, then we notify, wait kLowMemoryCheckTimeoutMs milliseconds and then
// start watching again to see if we're still in a low memory state.  This is to
// keep from sending out multiple notifications before the UI has a chance to
// respond (it may take the UI a while to actually deallocate memory). A timer
// isn't the perfect solution, but without any reliable indicator that a tab has
// had all its parts deallocated, it's the next best thing.
class LowMemoryListenerImpl
    : public base::RefCountedThreadSafe<LowMemoryListenerImpl> {
 public:
  // LowMemoryListenerImpl() : watcher_delegate_(this), file_descriptor_(-1) {}
  LowMemoryListenerImpl(int oomEventFd) : watcher_delegate_(this), file_descriptor_(oomEventFd) {}

  // Start watching the low memory file for readability.
  // Calls to StartObserving should always be matched with calls to
  // StopObserving.  This method should only be called from the FILE thread.
  // |low_memory_callback| is run when memory is low.
  void StartObservingOnFileThread(const base::Closure& low_memory_callback);

  // Stop watching the low memory file for readability.
  // May be safely called if StartObserving has not been called.
  // This method should only be called from the FILE thread.
  void StopObservingOnFileThread();

 private:
  friend class base::RefCountedThreadSafe<LowMemoryListenerImpl>;

  ~LowMemoryListenerImpl() {
    StopObservingOnFileThread();
  }

  // Start a timer to resume watching the low memory file descriptor.
  void ScheduleNextObservation();

  // Actually start watching the file descriptor.
  void StartWatchingDescriptor();

  // Delegate to receive events from WatchFileDescriptor.
  class FileWatcherDelegate : public base::MessageLoopForIO::Watcher {
   public:
    explicit FileWatcherDelegate(LowMemoryListenerImpl* owner)
        : owner_(owner) {}
    virtual ~FileWatcherDelegate() {}

    // Overrides for MessageLoopForIO::Watcher
    virtual void OnFileCanWriteWithoutBlocking(int fd) OVERRIDE {
    }
    virtual void OnFileCanReadWithoutBlocking(int fd) OVERRIDE {
      LOG(ERROR) << "Low memory condition detected.  Discarding a tab.";
      // We can only discard tabs on the UI thread.
      BrowserThread::PostTask(BrowserThread::UI,
                              FROM_HERE,
                              owner_->low_memory_callback_);
      owner_->ScheduleNextObservation();
    }

   private:
    LowMemoryListenerImpl* owner_;
    DISALLOW_COPY_AND_ASSIGN(FileWatcherDelegate);
  };

  scoped_ptr<base::MessageLoopForIO::FileDescriptorWatcher> watcher_;
  FileWatcherDelegate watcher_delegate_;
  int file_descriptor_;
  base::OneShotTimer<LowMemoryListenerImpl> timer_;
  base::Closure low_memory_callback_;

  DISALLOW_COPY_AND_ASSIGN(LowMemoryListenerImpl);
};

void LowMemoryListenerImpl::StartObservingOnFileThread(
    const base::Closure& low_memory_callback) {
  LOG(ERROR) << "LowMemoryListenerImpl::StartObservingOnFileThread()";
  low_memory_callback_ = low_memory_callback;
  DCHECK(watcher_.get() == NULL);
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  DCHECK(base::MessageLoopForIO::current());

  LOG(ERROR) << "file_descriptor_ " << file_descriptor_;
  // Don't report this error unless we're really running on ChromeOS
  // to avoid testing spam.
  if (file_descriptor_ < 0) {
    LOG(ERROR) << "Unable to open " ;
    return;
  }
  watcher_.reset(new base::MessageLoopForIO::FileDescriptorWatcher);
  StartWatchingDescriptor();
}

void LowMemoryListenerImpl::StopObservingOnFileThread() {
  // If StartObserving failed, StopObserving will still get called.
  timer_.Stop();
  if (file_descriptor_ >= 0) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
    watcher_.reset(NULL);
    ::close(file_descriptor_);
    file_descriptor_ = -1;
  }
}

void LowMemoryListenerImpl::ScheduleNextObservation() {
  timer_.Start(FROM_HERE,
               base::TimeDelta::FromMilliseconds(kLowMemoryCheckTimeoutMs),
               this,
               &LowMemoryListenerImpl::StartWatchingDescriptor);
}

void LowMemoryListenerImpl::StartWatchingDescriptor() {
  DCHECK(watcher_.get());
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  DCHECK(base::MessageLoopForIO::current());
  if (file_descriptor_ < 0)
    return;
  if (!base::MessageLoopForIO::current()->WatchFileDescriptor(
          file_descriptor_,
          false,  // persistent=false: We want it to fire once and reschedule.
          base::MessageLoopForIO::WATCH_READ,
          watcher_.get(),
          &watcher_delegate_)) {
    LOG(ERROR) << "Unable to watch " << file_descriptor_;
  }
}

////////////////////////////////////////////////////////////////////////////////
// LowMemoryListener

LowMemoryListener::LowMemoryListener(LowMemoryListenerDelegate* delegate, int oomEventFd)
    : observer_(new LowMemoryListenerImpl(oomEventFd)),
      delegate_(delegate),
      weak_factory_(this) {
  LOG(ERROR) << "LowMemoryListener::LowMemoryListener()";
}
LowMemoryListener::~LowMemoryListener() {
  Stop();
}

void LowMemoryListener::Start() {
  LOG(ERROR) << "LowMemoryListener::Start()";
  base::Closure memory_low_callback =
      base::Bind(&LowMemoryListener::OnMemoryLow, weak_factory_.GetWeakPtr());
  BrowserThread::PostTask(
      BrowserThread::FILE,
      FROM_HERE,
      base::Bind(&LowMemoryListenerImpl::StartObservingOnFileThread,
                 observer_.get(),
                 memory_low_callback));
}

void LowMemoryListener::Stop() {
  LOG(ERROR) << "LowMemoryListener::Stop()";
  weak_factory_.InvalidateWeakPtrs();
  BrowserThread::PostTask(
      BrowserThread::FILE,
      FROM_HERE,
      base::Bind(&LowMemoryListenerImpl::StopObservingOnFileThread,
                 observer_.get()));
}

void LowMemoryListener::OnMemoryLow() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  LOG(ERROR) << "LowMemoryListener::OnMemoryLow()";
  delegate_->OnMemoryLow();
}

}  // namespace content
