// Copyright (c) 2014 ACCESS CO., LTD. All rights reserved.

#ifndef CONTENT_SHELL_RENDERER_ACS_ACS_WEB_CONTENTS_OBSERVER_H_
#define CONTENT_SHELL_RENDERER_ACS_ACS_WEB_CONTENTS_OBSERVER_H_

#include <vector>
#include <string>
#include <list>

#include "base/timer/timer.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {

typedef std::list<std::string> AutoTestLinksList;

// this class reacts on WebContents events and
// runs a series of tests by opening subsequently the links given in
// the file 'sitelist.txt' waiting until it finishes or fails loading
// and then, after 10s, loading the next link
// this can be activated using the --acs-autotest command line parameter
//
// this class is currently limited to loading pages in one window
// if multiple windows are started, then multiple autotest cycles may be started
class AcsWebContentsObserver : public WebContentsObserver,
                               public WebContentsUserData<AcsWebContentsObserver> {
 public:
  ~AcsWebContentsObserver() final;

  // WebContentsObserver implementation.

  // If the WebContents is displaying replacement content, e.g. network error
  // pages, DidFinishLoad is invoked for frames that were not sending
  // navigational events before. It is safe to ignore these events.
  void DidFinishLoad(RenderFrameHost* render_frame_host,
                             const GURL& validated_url) override;

  // This method is like DidFinishLoad, but when the load failed or was
  // cancelled, e.g. window.stop() is invoked.
  void DidFailLoad(RenderFrameHost* render_frame_host,
                           const GURL& validated_url,
                           int error_code,
                           const base::string16& error_description,
                           bool was_ignored_by_handler) override;

 private:
  friend class WebContentsUserData<AcsWebContentsObserver>;

  explicit AcsWebContentsObserver(WebContents* web_contents);

  bool autotest_finished_;

  // how many times should we test all the links from sitelist.txt
  unsigned int autotest_loops_;

  // current loop counter
  unsigned int autotest_loop_counter_;

  AutoTestLinksList autotest_links_;
  AutoTestLinksList::const_iterator autotest_links_itr_;

  // The timer to check if no Finish or FailLoad was called.
  base::OneShotTimer timeout_timer_;

  // tries to move the autotest_links_itr_ to the next valid link
  // returns false if there are no more loops to go and reached the end
  //               of the links vector
  bool iterateToNextValidLink();

  // Loads the AutoTestLinksList on the FILE thread.
  void LoadLinksList();

  // loads the URL pointed by autotest_links_itr_
  void LoadNextURL();

  // posts a task that will call LoadNextURL()
  void TriggerNextLoad();

  // Writes a report line if a page load takes more than kDefaultPageTimeoutInSeconds
  void TimeoutLoad();

  // to be called by the PerformanceMonitorLight after gathering data
  // it will post a task to the FILE thread which will update the report file
  // with the given report data
  void UpdateReportWithData(size_t priv_mem_usage,
          size_t shared_mem_usage, double cpu_usage);

  // sets the PerformanceMonitorLight to call UpdateReportWithData after
  // the next timed data collection cycle
  void StartReportUpdating();

  // task to run on the file io thread
  // dumps the data prepared previously into the report file
  void FinishReportUpdating();

  // storing the data for one line of the report
  struct ReportLineData {
      unsigned long int line_;
      base::Time start_time_;
      base::Time stop_time_;
      size_t priv_mem_usage_;
      size_t shared_mem_usage_;
      size_t sys_mem_usage_;
      double cpu_usage_;
      int error_code_;
      std::string url_;
      ReportLineData();
  };

  // multiple functions are filling this in with the report data
  // FinishReportUpdating is writing it to the
  ReportLineData current_report_line_;
  // FIXME: there is no guarantee that the next test is not overwriting the data
  // from the previous test

  // full name of the autotest report file
  // e.g. blink_autotest_201407030914.txt
  std::string autotest_report_filename_;

  // prevents starting the test cycle in subsequent WebContents
  // the autotest should be started only once in the first created window
  static bool autotest_started_;

  DISALLOW_COPY_AND_ASSIGN(AcsWebContentsObserver);
};

}  // namespace content


#endif  // CONTENT_SHELL_RENDERER_ACS_ACS_WEB_CONTENTS_OBSERVER_H_
