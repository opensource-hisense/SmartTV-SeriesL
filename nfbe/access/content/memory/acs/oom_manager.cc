// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "oom_manager.h"

#include <algorithm>
#include <set>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <sys/syscall.h>

#include "low_memory_listener.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"

using base::TimeDelta;
using base::TimeTicks;
using content::BrowserThread;

namespace content {

namespace {
static inline int eventfd(unsigned count, unsigned dummy) {
    return syscall(SYS_eventfd, count, dummy);
}
const char eventControlFile[] = "/sys/fs/cgroup/browser/content_shell/cgroup.event_control";
const char oomControlFile[] = "/sys/fs/cgroup/browser/content_shell/memory.oom_control";
const char pressureLevelFile[] = "/sys/fs/cgroup/browser/content_shell/memory.pressure_level";
class LowMemoryListenerDelegateMedium : public LowMemoryListenerDelegate {
 public:
  virtual void OnMemoryLow() OVERRIDE;
};
class LowMemoryListenerDelegateCritical : public LowMemoryListenerDelegate {
 public:
  virtual void OnMemoryLow() OVERRIDE;
};
} //namespace

////////////////////////////////////////////////////////////////////////////////
// OomManager

OomManager::OomManager() {
  /* FIXME: need FD_CLOEXEC?  probably. */
  oomEventFd = eventfd(0, 0);
  mediumEventFd = eventfd(0, 0);
  criticalEventFd = eventfd(0, 0);
  setOomControl();
  low_memory_listener_.reset(new LowMemoryListener(this, oomEventFd));
  delegate_1_ = new LowMemoryListenerDelegateMedium();
  low_memory_listener_1_.reset(new LowMemoryListener(delegate_1_, mediumEventFd));
  delegate_2_ = new LowMemoryListenerDelegateCritical();
  low_memory_listener_2_.reset(new LowMemoryListener(delegate_2_, criticalEventFd));
  LOG(ERROR) << "OomManager::OomManager()";
}

OomManager::~OomManager() {
  Stop();
}

void OomManager::Start() {
  LOG(ERROR) << "OomManager::Start()";
  if (low_memory_listener_.get())
    low_memory_listener_->Start();
  if (low_memory_listener_1_.get())
    low_memory_listener_1_->Start();
  if (low_memory_listener_2_.get())
    low_memory_listener_2_->Start();
}

void OomManager::Stop() {
  LOG(ERROR) << "OomManager::Stop()";
  if (low_memory_listener_.get())
    low_memory_listener_->Stop();
  if (low_memory_listener_1_.get())
    low_memory_listener_1_->Stop();
  if (low_memory_listener_2_.get())
    low_memory_listener_2_->Stop();
}

void OomManager::OnMemoryLow() {
  LOG(ERROR) << "OomManager::OnMemoryLow() UI Thread = " << BrowserThread::CurrentlyOn(BrowserThread::UI);
  // LogMemoryAndDiscardTab();
}
void OomManager::setOomControl() {
  LOG(ERROR) << "OomManager::setOomControl()";
  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&OomManager::setOomControlOnFileThread,
                 base::Unretained(this), oomEventFd, mediumEventFd, criticalEventFd));
}

void OomManager::setOomControlOnFileThread(int oomEventFd, int mediumEventFd, int criticalEventFd) {
  LOG(ERROR) << "OomManager::setOomControlOnFileThread() run on file thread= " << BrowserThread::CurrentlyOn(BrowserThread::FILE);
  LOG(ERROR) << "oomEventFd is " << oomEventFd;
  int eventControlFd;
  /* FIXME: who closes oomControlFd */
  int oomControlFd;
  /* FIXME: who closes pressureLevelFd */
  int pressureLevelFd;
  int wb;
  int result;
  char buf[LINE_MAX];

  eventControlFd = open(eventControlFile, O_WRONLY);
  if (eventControlFd == -1) {
    LOG(ERROR) << "Failed to open eventControlFd";
    return;
  }

  oomControlFd = open(oomControlFile, O_RDONLY);
  if (oomControlFd == -1) {
    LOG(ERROR) << "Failed to open oomControlFd";
    close(eventControlFd);
    return;
  }

  wb = snprintf(buf, 64, "%d %d", oomEventFd, oomControlFd);
  if (wb >= 64) {
    LOG(ERROR) << "buf size is samll";
    close(oomControlFd);
    close(eventControlFd);
    return;
  }
  LOG(ERROR) << "wb = " << wb;
  LOG(ERROR) << "buf = " << buf;
  LOG(ERROR) << "eventControlFd = " << eventControlFd;
  LOG(ERROR) << "oomControlFd = " << oomControlFd;

  result = write(eventControlFd, buf, wb + 1);
  if (result == -1) {
    LOG(ERROR) << "Failed to write eventControlFd";
    return;
  }

  if (-1 != mediumEventFd && -1 != criticalEventFd) {
    pressureLevelFd = open(pressureLevelFile, O_RDONLY);
    if (-1 == pressureLevelFd) {
      LOG(ERROR) << "Failed to open " << pressureLevelFile;
    } else {
      if (-1 != mediumEventFd) {
        wb = snprintf(buf, LINE_MAX, "%d %d medium", mediumEventFd, pressureLevelFd);
//      wb = snprintf(buf, LINE_MAX, "%d %d low", mediumEventFd, pressureLevelFd);
        if (LINE_MAX <= wb) {
          /*EMPTY*/
        } else {
          result = write(eventControlFd, buf, wb + 1);
        }
      }
      if (-1 != criticalEventFd) {
        wb = snprintf(buf, LINE_MAX, "%d %d critical", criticalEventFd, pressureLevelFd);
        if (LINE_MAX <= wb) {
          /*EMPTY*/
        } else {
          result = write(eventControlFd, buf, wb + 1);
        }
      }
    }
  }

  result = close(eventControlFd);
  if (result == -1) {
    LOG(ERROR) << "Failed to close eventControlFd";
    return;
  }
}


void LowMemoryListenerDelegateMedium::OnMemoryLow() {
  RenderProcessHost::iterator it = RenderProcessHost::AllHostsIterator();
  while (!it.IsAtEnd()) {
    int renderer_id = it.GetCurrentValue()->GetID();
    RenderProcessHost* proc_host = RenderProcessHost::FromID(renderer_id);
    RenderProcessHostImpl* proc_host_impl = static_cast<RenderProcessHostImpl*>(proc_host);
    proc_host_impl->NotifyLowMemory(1);
    it.Advance();
  }
}

void LowMemoryListenerDelegateCritical::OnMemoryLow() {
  RenderProcessHost::iterator it = RenderProcessHost::AllHostsIterator();
  while (!it.IsAtEnd()) {
    int renderer_id = it.GetCurrentValue()->GetID();
    RenderProcessHost* proc_host = RenderProcessHost::FromID(renderer_id);
    RenderProcessHostImpl* proc_host_impl = static_cast<RenderProcessHostImpl*>(proc_host);
    proc_host_impl->NotifyLowMemory(2);
    it.Advance();
  }
}

}  // namespace content
