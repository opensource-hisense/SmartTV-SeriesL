// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2014 ACCESS CO., LTD. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_RENDERER_ACS_PERFORMANCE_MONITOR_LIGHT_H_
#define CONTENT_SHELL_RENDERER_ACS_PERFORMANCE_MONITOR_LIGHT_H_

#include <map>
#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "base/process/process_metrics.h"
#include "base/timer/timer.h"
#include "content/shell/browser/acs/memory_details.h"


namespace performance_monitor_light {

// The default interval at which PerformanceMonitorLight performs its timed
// collections; this can be overridden by using the kPerformanceMonitorLight
// switch with an associated (positive integer) value.
const int kDefaultGatherIntervalInSeconds = 30;


// PerformanceMonitorLight is a tool which will allow the user to view information
// about content_shell's performance over a period of time. It will gather statistics
// pertaining to performance-oriented areas (e.g. memory usage).
//
// The collection is done using a helper class derived from MemoryDetails.
//
// Thread Safety: PerformanceMonitorLight lives on multiple threads.
// The PerformanceMonitorLight will act on the appropriate thread for
// the task (for instance, gathering statistics about CPU and memory usage
// is done on the background thread, but most notifications occur on the UI
// thread).
class PerformanceMonitorLight {
 public:
  // Callback when the data was collected.
  // Args: total private memory used, total shared memory used, total cpu_usage
  typedef base::Callback<void(size_t, size_t, double)> MemoryUsageCollectedCB;


  // Returns the current PerformanceMonitorLight instance if one exists; otherwise
  // constructs a new PerformanceMonitorLight.
  static PerformanceMonitorLight* GetInstance();

  static bool initialized() { return initialized_; }

  // Perform any collections that are done on a separate thread.
  void DoTimedCollections();

  // Store the main process ID for later use
  // Must be called once from the content_shell starting process.
  void Initialize(const bool dump2screen=true, const int gather_interval=kDefaultGatherIntervalInSeconds);

  // Set the callback task that should be called when the next gathering is finished
  // This is used to get total memory consumption data
  // outside of the Performance Monitor Light
  void SetCollectionCB(const MemoryUsageCollectedCB& memory_usage_collected_cb);

 private:
  friend struct base::DefaultSingletonTraits<PerformanceMonitorLight>;

  PerformanceMonitorLight();
  virtual ~PerformanceMonitorLight();

  // Perform any additional initialization which must be performed on a
  // background thread (e.g. starting the timer).
  void FinishInit();

  // Parse the process data and calculate total memory consumption.
  // Calls the memory_usage_collected_cb_.
  // It's getting called when the Memory Details helper class is ready with
  // the collection of the data.
  void ProcessMemoryData(const struct ProcessData& process_data);

  // A flag indicating whether or not PerformanceMonitorLight is initialized. Any
  // external sources accessing PerformanceMonitorLight should check this flag.
  static bool initialized_;

  // The timer to signal PerformanceMonitorLight to perform its timed collections.
  base::RepeatingTimer timer_;

  // A flag indicating if the gathered data should be dumped to screen or not
  // by default true
  bool dump2screen_;

  // A flag indicating that by the next memory usage collection we should call
  // the registered callback
  bool call_mem_cb_next_;

  // Called when the total memory data was collected by this class.
  // Can be set with SetCollectionCB() and it passes the data about the
  // total memory consumption to outside classes
  MemoryUsageCollectedCB memory_usage_collected_cb_;

  // Can be set by giving the second parameter to Initialize()
  int gather_interval_in_seconds_;

  DISALLOW_COPY_AND_ASSIGN(PerformanceMonitorLight);
};

}  // namespace performance_monitor_light

#endif  // CONTENT_SHELL_RENDERER_ACS_PERFORMANCE_MONITOR_LIGHT_H_
