// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_MEMORY_ACS_OOM_MANAGER_H_
#define CONTENT_MEMORY_ACS_OOM_MANAGER_H_

#include <vector>
//
#include "base/compiler_specific.h"
#include "base/containers/hash_tables.h"
#include "base/memory/scoped_ptr.h"
#include "base/process/process.h"
#include "base/strings/string16.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "low_memory_listener_delegate.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

// class GURL;

namespace content {

class LowMemoryListener;

class OomManager : public LowMemoryListenerDelegate {
 public:
  OomManager();
  virtual ~OomManager();

  void Start();
  void Stop();

 private:

  // LowMemoryListenerDelegate overrides:
  virtual void OnMemoryLow() OVERRIDE;

  // Observer for the kernel low memory signal.
  scoped_ptr<LowMemoryListener> low_memory_listener_;

  int mediumEventFd;
  int criticalEventFd;
  LowMemoryListenerDelegate* delegate_1_;
  LowMemoryListenerDelegate* delegate_2_;
  scoped_ptr<LowMemoryListener> low_memory_listener_1_;
  scoped_ptr<LowMemoryListener> low_memory_listener_2_;

  int oomEventFd;
  void setOomControlOnFileThread(int oomEventFd, int mediumEventFd, int criticalEventFd);
  void setOomControl();

  DISALLOW_COPY_AND_ASSIGN(OomManager);
};

}  // namespace content

#endif  // CONTENT_MEMORY_ACS_OOM_MANAGER_H_
