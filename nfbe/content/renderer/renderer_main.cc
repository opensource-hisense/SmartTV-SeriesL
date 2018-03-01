// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <utility>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/debug/debugger.h"
#include "base/debug/leak_annotations.h"
#include "base/feature_list.h"
#include "base/i18n/rtl.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/statistics_recorder.h"
#include "base/pending_task.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/sys_info.h"
#include "base/thread_task_runner_handle.h"
#include "base/threading/platform_thread.h"
#include "base/time/default_tick_clock.h"
#include "base/timer/hi_res_timer_manager.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "components/scheduler/renderer/renderer_scheduler.h"
#include "components/memory_pressure/direct_memory_pressure_calculator_linux.h"
#include "components/memory_pressure/memory_pressure_listener.h"
#include "components/memory_pressure/memory_pressure_monitor.h"
#include "components/memory_pressure/memory_pressure_stats_collector.h"
#include "content/child/child_process.h"
#include "content/common/content_constants_internal.h"
#include "content/common/mojo/mojo_shell_connection_impl.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/renderer/render_process_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/renderer_main_platform_delegate.h"
#include "third_party/skia/include/core/SkGraphics.h"
#include "ui/base/ui_base_switches.h"

#if defined(OS_ANDROID)
#include "base/android/library_loader/library_loader_hooks.h"
#endif  // OS_ANDROID

#if defined(OS_MACOSX)
#include <Carbon/Carbon.h>
#include <signal.h>
#include <unistd.h>

#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/message_loop/message_pump_mac.h"
#include "third_party/WebKit/public/web/WebView.h"
#endif  // OS_MACOSX

#if defined(ENABLE_PLUGINS)
#include "content/renderer/pepper/pepper_plugin_registry.h"
#endif

#if defined(ENABLE_WEBRTC)
#include "third_party/libjingle/overrides/init_webrtc.h"
#endif

#if defined(USE_OZONE)
#include "ui/ozone/public/client_native_pixmap_factory.h"
#endif

namespace content {
namespace {
// This function provides some ways to test crash and assertion handling
// behavior of the renderer.
static void HandleRendererErrorTestParameters(
    const base::CommandLine& command_line) {
  if (command_line.HasSwitch(switches::kWaitForDebugger))
    base::debug::WaitForDebugger(60, true);

  if (command_line.HasSwitch(switches::kRendererStartupDialog))
    ChildProcess::WaitForDebugger("Renderer");
}

#if defined(USE_OZONE)
base::LazyInstance<scoped_ptr<ui::ClientNativePixmapFactory>> g_pixmap_factory =
    LAZY_INSTANCE_INITIALIZER;
#endif

// MemoryPressureFixer doesn't use FilteredMemoryPressureCalculator class.
// Because it doesn't contribute to the memory usage reduction, rather just
// wastes the CPU resource in the order of a few handred milliseconds.
class MemoryPressureFixer final {
 public:
  static MemoryPressureFixer* Create(
      const base::CommandLine& parsed_command_line) {
    // The following code was copied from CreateWinMemoryPressureMonitor() in
    // content/browser/browser_main_loop.cc.
    std::vector<std::string> thresholds = base::SplitString(
        parsed_command_line.GetSwitchValueASCII(
            switches::kMemoryPressureThresholdsMb),
        ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    int moderate_threshold_mb = 0;
    int critical_threshold_mb = 0;
    if (thresholds.size() == 2 &&
        base::StringToInt(thresholds[0], &moderate_threshold_mb) &&
        base::StringToInt(thresholds[1], &critical_threshold_mb) &&
        moderate_threshold_mb >= critical_threshold_mb &&
        critical_threshold_mb >= 0) {
      return new MemoryPressureFixer(
          moderate_threshold_mb, critical_threshold_mb);
    }
    return nullptr;
  }

  ~MemoryPressureFixer() = default;

 private:
  using MemoryPressureMonitor = memory_pressure::MemoryPressureMonitor;
  using MemoryPressureListener = memory_pressure::MemoryPressureListener;

  static void NotifyMemoryPressure(
      MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
    LOG(INFO) << "MemoryPressureFixer: level: " << memory_pressure_level;
    MemoryPressureListener::NotifyMemoryPressure(memory_pressure_level);
  }

  MemoryPressureFixer(int moderator_threshold_mb, int critical_threshold_mb)
      : stats_collector_(&tick_clock_),
        direct_calculator_(moderator_threshold_mb, critical_threshold_mb) {
    monitor_.reset(new MemoryPressureMonitor(
        base::ThreadTaskRunnerHandle::Get(), &tick_clock_, &stats_collector_,
        &direct_calculator_, base::Bind(&NotifyMemoryPressure)));
  }

  base::DefaultTickClock tick_clock_;
  memory_pressure::MemoryPressureStatsCollector stats_collector_;
  memory_pressure::DirectMemoryPressureCalculator direct_calculator_;
  scoped_ptr<MemoryPressureMonitor> monitor_;

};

}  // namespace

// mainline routine for running as the Renderer process
int RendererMain(const MainFunctionParams& parameters) {
  // Don't use the TRACE_EVENT0 macro because the tracing infrastructure doesn't
  // expect synchronous events around the main loop of a thread.
  TRACE_EVENT_ASYNC_BEGIN0("startup", "RendererMain", 0);

  base::trace_event::TraceLog::GetInstance()->SetProcessName("Renderer");
  base::trace_event::TraceLog::GetInstance()->SetProcessSortIndex(
      kTraceEventRendererProcessSortIndex);

  const base::CommandLine& parsed_command_line = parameters.command_line;

  MojoShellConnectionImpl::Create();

#if defined(OS_MACOSX)
  base::mac::ScopedNSAutoreleasePool* pool = parameters.autorelease_pool;
#endif  // OS_MACOSX

#if defined(OS_CHROMEOS)
  // As Zygote process starts up earlier than browser process gets its own
  // locale (at login time for Chrome OS), we have to set the ICU default
  // locale for renderer process here.
  // ICU locale will be used for fallback font selection etc.
  if (parsed_command_line.HasSwitch(switches::kLang)) {
    const std::string locale =
        parsed_command_line.GetSwitchValueASCII(switches::kLang);
    base::i18n::SetICUDefaultLocale(locale);
  }
#endif

  SkGraphics::Init();
#if defined(OS_ANDROID)
  const int kMB = 1024 * 1024;
  size_t font_cache_limit =
      base::SysInfo::IsLowEndDevice() ? kMB : 8 * kMB;
  SkGraphics::SetFontCacheLimit(font_cache_limit);
#endif

#if defined(USE_OZONE)
  g_pixmap_factory.Get() = ui::ClientNativePixmapFactory::Create();
  ui::ClientNativePixmapFactory::SetInstance(g_pixmap_factory.Get().get());
#endif

  // This function allows pausing execution using the --renderer-startup-dialog
  // flag allowing us to attach a debugger.
  // Do not move this function down since that would mean we can't easily debug
  // whatever occurs before it.
  HandleRendererErrorTestParameters(parsed_command_line);

  RendererMainPlatformDelegate platform(parameters);
#if defined(OS_MACOSX)
  // As long as scrollbars on Mac are painted with Cocoa, the message pump
  // needs to be backed by a Foundation-level loop to process NSTimers. See
  // http://crbug.com/306348#c24 for details.
  scoped_ptr<base::MessagePump> pump(new base::MessagePumpNSRunLoop());
  scoped_ptr<base::MessageLoop> main_message_loop(
      new base::MessageLoop(std::move(pump)));
#else
  // The main message loop of the renderer services doesn't have IO or UI tasks.
  scoped_ptr<base::MessageLoop> main_message_loop(new base::MessageLoop());
#endif

  base::PlatformThread::SetName("CrRendererMain");

  bool no_sandbox = parsed_command_line.HasSwitch(switches::kNoSandbox);

  // Initialize histogram statistics gathering system.
  base::StatisticsRecorder::Initialize();

#if defined(OS_ANDROID)
  // If we have a pending chromium android linker histogram, record it.
  base::android::RecordChromiumAndroidLinkerRendererHistogram();
#endif

  // Initialize statistical testing infrastructure.  We set the entropy provider
  // to NULL to disallow the renderer process from creating its own one-time
  // randomized trials; they should be created in the browser process.
  base::FieldTrialList field_trial_list(NULL);
  // Ensure any field trials in browser are reflected into renderer.
  if (parsed_command_line.HasSwitch(switches::kForceFieldTrials)) {
    bool result = base::FieldTrialList::CreateTrialsFromString(
        parsed_command_line.GetSwitchValueASCII(switches::kForceFieldTrials),
        std::set<std::string>());
    DCHECK(result);
  }

  scoped_ptr<base::FeatureList> feature_list(new base::FeatureList);
  feature_list->InitializeFromCommandLine(
      parsed_command_line.GetSwitchValueASCII(switches::kEnableFeatures),
      parsed_command_line.GetSwitchValueASCII(switches::kDisableFeatures));
  base::FeatureList::SetInstance(std::move(feature_list));

  scoped_ptr<scheduler::RendererScheduler> renderer_scheduler(
      scheduler::RendererScheduler::Create());

  // PlatformInitialize uses FieldTrials, so this must happen later.
  platform.PlatformInitialize();

#if defined(ENABLE_PLUGINS)
  // Load pepper plugins before engaging the sandbox.
  PepperPluginRegistry::GetInstance();
#endif
#if defined(ENABLE_WEBRTC)
  // Initialize WebRTC before engaging the sandbox.
  // NOTE: On linux, this call could already have been made from
  // zygote_main_linux.cc.  However, calling multiple times from the same thread
  // is OK.
  InitializeWebRtcModule();
#endif

  {
#if defined(OS_WIN) || defined(OS_MACOSX)
    // TODO(markus): Check if it is OK to unconditionally move this
    // instruction down.
    RenderProcessImpl render_process;
    RenderThreadImpl::Create(std::move(main_message_loop),
                             std::move(renderer_scheduler));
#endif
    bool run_loop = true;
    if (!no_sandbox)
      run_loop = platform.EnableSandbox();
#if defined(OS_POSIX) && !defined(OS_MACOSX)
    RenderProcessImpl render_process;
    RenderThreadImpl::Create(std::move(main_message_loop),
                             std::move(renderer_scheduler));
#endif

    base::HighResolutionTimerManager hi_res_timer_manager;

    if (run_loop) {
      scoped_ptr<MemoryPressureFixer> memory_pressure_fixer(
          MemoryPressureFixer::Create(parsed_command_line));
#if defined(OS_MACOSX)
      if (pool)
        pool->Recycle();
#endif
      TRACE_EVENT_ASYNC_BEGIN0("toplevel", "RendererMain.START_MSG_LOOP", 0);
      base::MessageLoop::current()->Run();
      TRACE_EVENT_ASYNC_END0("toplevel", "RendererMain.START_MSG_LOOP", 0);
    }

    MojoShellConnectionImpl::Destroy();

#if defined(LEAK_SANITIZER)
    // Run leak detection before RenderProcessImpl goes out of scope. This helps
    // ignore shutdown-only leaks.
    __lsan_do_leak_check();
#endif
  }
  platform.PlatformUninitialize();
  TRACE_EVENT_ASYNC_END0("startup", "RendererMain", 0);
  return 0;
}

}  // namespace content
