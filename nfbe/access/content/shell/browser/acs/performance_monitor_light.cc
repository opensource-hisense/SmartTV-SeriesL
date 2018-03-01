// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2014 ACCESS CO., LTD. All rights reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "access/content/shell/browser/acs/performance_monitor_light.h"

#include <set>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/worker_pool.h"
#include "base/time/time.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/load_notification_details.h"
#include "content/public/common/content_switches.h"
#include "content/shell/browser/acs/memory_details.h"

using content::BrowserThread;

namespace performance_monitor_light {

// Internal callback getting called when the helper Memory Details
// is ready collection memory data for each process.
typedef base::Callback<void(const ProcessData&)> ProcessDataReadyCB;

// This is a helper class collecting memory details and CPU usage values.
class NFBEMemoryDetails : public MemoryDetails {
public:
    NFBEMemoryDetails(const ProcessDataReadyCB& cb, bool log_details) :
        process_data_collected_cb_(cb),
        log_details_(log_details) {

        AddRef();  // Released in OnDetailsAvailable().
    }

    void OnDetailsAvailable() override {
        if (log_details_) {
            std::string log_string = ToLogString();
            LOG(INFO) << " MEMORY Details :\n"
                      << log_string;
        }

        const ProcessData& browser = *ChromeBrowser();
        process_data_collected_cb_.Run(browser);
        Release();
    }

    static void CollectAndCallback(const ProcessDataReadyCB& cb,
                                    bool log_details) {
        // Deletes itself upon completion.
        NFBEMemoryDetails* details_collector =
            new NFBEMemoryDetails(cb, log_details);
        details_collector->StartFetch(MemoryDetails::FROM_CHROME_ONLY);
    }

private:
    ~NFBEMemoryDetails() override {};

    // Called when the data about each process was collected.
    // That is after OnDetailsAvailable()
    ProcessDataReadyCB process_data_collected_cb_;

    // Print out a string with the memory consumption for each process
    bool log_details_;
};



bool PerformanceMonitorLight::initialized_ = false;

PerformanceMonitorLight::PerformanceMonitorLight()
    : dump2screen_(true),
      call_mem_cb_next_(false),
      gather_interval_in_seconds_(kDefaultGatherIntervalInSeconds) {
}

PerformanceMonitorLight::~PerformanceMonitorLight() {
}

// static
PerformanceMonitorLight* PerformanceMonitorLight::GetInstance() {
    return base::Singleton<PerformanceMonitorLight>::get();
}


void PerformanceMonitorLight::Initialize(const bool dump2screen,
        const int gather_interval) {
    if (!initialized_) {
        DLOG(INFO) << __FUNCTION__ << " with browser_pid " << base::GetCurrentProcId();
        DLOG(INFO) << __FUNCTION__ << " ui thread "
                   << BrowserThread::IsThreadInitialized(BrowserThread::UI);
        dump2screen_ = dump2screen;
        gather_interval_in_seconds_ = gather_interval;
        // start this a little bit later so that all the timer threads are in place
        content::BrowserThread::GetMessageLoopProxyForThread(BrowserThread::UI)->
                PostDelayedTask(
                    FROM_HERE,
                    base::Bind(&PerformanceMonitorLight::FinishInit,
                                base::Unretained(this)),
                    base::TimeDelta::FromSeconds(5));
    }
}


void PerformanceMonitorLight::FinishInit() {
    CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kPerformanceMonitorLight)
        && !base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            switches::kPerformanceMonitorLight).empty()) {
        int specified_interval = 0;
        if (!base::StringToInt(
                base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                    switches::kPerformanceMonitorLight),
                    &specified_interval)
            || specified_interval <= 0) {
          LOG(ERROR) << "Invalid value for switch: '"
                     << switches::kPerformanceMonitorLight
                     << "'; please use an integer greater than 0.";
        } else {
            gather_interval_in_seconds_ = specified_interval;
        }
    }

    LOG(INFO) << __FUNCTION__ << " Performance Monitor Light running at "
              << gather_interval_in_seconds_ << "s";

    timer_.Start(FROM_HERE,
                 base::TimeDelta::FromSeconds(gather_interval_in_seconds_),
                 this,
                 &PerformanceMonitorLight::DoTimedCollections);
    initialized_ = true;
}


void PerformanceMonitorLight::SetCollectionCB(
        const MemoryUsageCollectedCB& memory_usage_collected_cb) {
    memory_usage_collected_cb_ = memory_usage_collected_cb;
    call_mem_cb_next_ = true;
}

void PerformanceMonitorLight::ProcessMemoryData(const ProcessData& process_data) {
    // handle data for each process
    size_t private_total = 0;
    size_t shared_total = 0;
    double cpu_usage = 0.0;
    for (size_t index = 0; index < process_data.processes.size(); index++) {
        private_total += process_data.processes[index].working_set.priv;
        shared_total += process_data.processes[index].working_set.shared;
        cpu_usage += process_data.processes[index].cpu_usage;
    }
    LOG(INFO) << " TOTAL MEMORY Details Private : " << private_total
              << " Shared : " << shared_total;

    if (call_mem_cb_next_) {
        call_mem_cb_next_ = false;
        memory_usage_collected_cb_.Run(private_total, shared_total, 0);
    }
}

void PerformanceMonitorLight::DoTimedCollections() {
    if (!initialized_) {
        DLOG(WARNING) << "You should call PerformanceMonitorLight::Initialize() in the main process once !!!";
        return;
    }
    NFBEMemoryDetails::CollectAndCallback(
        base::Bind(&PerformanceMonitorLight::ProcessMemoryData,
                   base::Unretained(this)),
        dump2screen_);

}


}  // namespace performance_monitor_light
