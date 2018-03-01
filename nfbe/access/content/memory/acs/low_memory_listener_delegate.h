// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_MEMORY_ACS_LOW_MEMORY_LISTENER_DELEGATE_H_
#define CONTENT_MEMORY_ACS_LOW_MEMORY_LISTENER_DELEGATE_H_

// #include "chromeos_memory_export.h"

namespace content {

class LowMemoryListenerDelegate {
 public:
  // Invoked when a low memory situation is detected.
  virtual void OnMemoryLow() = 0;

 protected:
  virtual ~LowMemoryListenerDelegate() {}
};

}  // namespace content

#endif  //CONTENT_MEMORY_ACS_LOW_MEMORY_LISTENER_DELEGATE_H_
