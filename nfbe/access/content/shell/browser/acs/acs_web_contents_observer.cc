// Copyright (c) 2014 ACCESS CO., LTD. All rights reserved.


#include "access/content/shell/browser/acs/acs_web_contents_observer.h"
#include "access/content/shell/browser/acs/performance_monitor_light.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/file_path.h"
#include "base/format_macros.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_switches.h"
#include "net/base/net_errors.h"
#include "ui/base/page_transition_types.h"

using namespace base;
using namespace performance_monitor_light;

namespace content {

// The default interval at which AcsWebContentsObserver loads a new page
const int kDefaultInterPageDelayInSeconds = 10;

// The default interval after which AcsWebContentsObserver loads
// the next page from the list.
const int kDefaultPageTimeoutInSeconds = 120;

// The default name of the file containing the test links
const char kDefaultAutoTestLinksFile[] = "sitelist.txt";

// The default name prefix and suffix for the file containing the test report
const char kAutoTestFileNamePrefix[] = "nfbe_autotest_";
const char kAutoTestFileNameSuffix[] = ".html";

DEFINE_WEB_CONTENTS_USER_DATA_KEY(AcsWebContentsObserver);

bool AcsWebContentsObserver::autotest_started_ = false;

AcsWebContentsObserver::ReportLineData::ReportLineData()
    : line_(0),
      priv_mem_usage_(0),
      shared_mem_usage_(0),
      sys_mem_usage_(0),
      cpu_usage_(0.0),
      error_code_(0)        {};



AcsWebContentsObserver::AcsWebContentsObserver(WebContents* web_contents)
    : WebContentsObserver(web_contents),
      autotest_finished_(false),
      autotest_loops_(1),
      autotest_loop_counter_(0) {

    if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kAcsAutoTest)
       && (!autotest_started_)) {
        autotest_started_ = true;
        // read from the cmd line how many loops we should perform
        if (!CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kAcsAutoTest).empty()) {
            int specified_loops = 0;
            if (!base::StringToInt(
                    CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                        switches::kAcsAutoTest),
                    &specified_loops) || specified_loops <= 0) {
              LOG(ERROR) << "Invalid value for switch: '"
                         << switches::kAcsAutoTest
                         << "'; please use an integer greater than 0.";
            } else {
                autotest_loops_ = specified_loops;
            }
        }
        LOG(INFO) << "Prepare to be running " << autotest_loops_
                  << " autotest loops";

        // FIXME: For very long lists or other corner cases
        // the link loading might take very long and so after the first loading
        // no pages are found in the autotesting list and the browser quits
        content::
          BrowserThread::
            GetMessageLoopProxyForThread(BrowserThread::FILE)->
                    PostTask(FROM_HERE,
                             base::Bind(&AcsWebContentsObserver::LoadLinksList,
                                        base::Unretained(this)));
    } else {
        autotest_loops_ = 0;
        LOG(INFO) << "NOT running AutoTest";
    }
}

AcsWebContentsObserver::~AcsWebContentsObserver() {}

void
AcsWebContentsObserver::LoadLinksList() {
    FilePath links_file(kDefaultAutoTestLinksFile);
    std::string links_list;
    std::vector<std::string> raw_links_vect;

    if (ReadFileToString(links_file, &links_list))
    {
      std::string delimiters;
      delimiters.push_back('\n');
      delimiters.push_back('\r');
      raw_links_vect = base::SplitString(
              links_list, delimiters, base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    }

    if (raw_links_vect.empty()) {
        LOG(ERROR) << "AutoTest links file " << kDefaultAutoTestLinksFile << " not found or empty !!!";
        autotest_loops_ = 0;
    } else {
        DLOG(INFO) << "List with AutoTest links : ";
        std::vector<std::string>::const_iterator raw_links_itr = raw_links_vect.begin();
        // parse the links and drop the comments or invalid ones
        while (raw_links_itr != raw_links_vect.end()) {
            if (((*raw_links_itr).c_str())[0] == '#' ) {
                DLOG(INFO) << "\t DROPPING COMMENT : " << (*raw_links_itr).c_str();
            } else {
                GURL url(*raw_links_itr);
                if (!url.is_valid()) {
                    LOG(WARNING) << "\t DROPPING INVALID : " << (*raw_links_itr).c_str();
                } else {
                    DLOG(INFO) << "\t ADDING VALID     : " << (*raw_links_itr).c_str();
                    autotest_links_.push_back(*raw_links_itr);
                }
            }
            raw_links_itr++;
        }

        if (autotest_links_.empty()) {
            autotest_loops_ = 0;
            LOG(ERROR) << "AutoTest links file " << kDefaultAutoTestLinksFile << " has no valid links !!!";
        } else {
            // set this to the last element in the vector
            // because iterateToNextValidLink() will increment it and
            // jump with it to the begining
            autotest_links_itr_ = autotest_links_.end();

            // start preparing the report file name
            autotest_report_filename_.append(kAutoTestFileNamePrefix);
        }
    }
}

void
AcsWebContentsObserver::LoadNextURL() {
    GURL url(*autotest_links_itr_);
    NavigationController::LoadURLParams params(url);
    params.transition_type = ui::PageTransitionFromInt(
        ui::PAGE_TRANSITION_TYPED | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR);
    params.should_clear_history_list = true;
    LOG(INFO) << "<1> AutoTest @ " << base::Time::Now().ToJavaTime() << "ms Starting on : "
              << url << " is_main_frame : 1";
    current_report_line_.start_time_ = base::Time::Now();
    web_contents()->GetController().LoadURLWithParams(params);

    // if the next link is not loaded after 2 minutes just move over to the next
    timeout_timer_.Start(FROM_HERE,
                         base::TimeDelta::FromSeconds(kDefaultPageTimeoutInSeconds),
                         base::Bind(&AcsWebContentsObserver::TimeoutLoad,
                                    base::Unretained(this)));
}

void
AcsWebContentsObserver::FinishReportUpdating() {
    const unsigned long int report_line_max_length = 1024;
    char report_line[report_line_max_length];
    std::string report_snippet;

    // first call is triggered after first FinishLoading
    // and the first FinishLoading corresponds to the page given on the command line
    // parse first line and write header
    if (0 == current_report_line_.line_) {
        report_snippet.append("<html><head><title>Auto Test log</title></head>\n");
        report_snippet.append("<h1>NFBE Autotest</h1>\n"
                              "<ul>\n");
        base::snprintf(report_line, report_line_max_length,
                            "<li>Cycles: %d</li>\n", autotest_loops_);
        report_snippet.append(report_line);

        // transform start time
        base::Time::Exploded t_struct;
        base::Time::Now().LocalExplode(&t_struct);
        base::snprintf(report_line, report_line_max_length,
                            "<li>StartTime: %4d/%02d/%02d %02d:%02d:%02d</li>\n",
                            t_struct.year,
                            t_struct.month,
                            t_struct.day_of_month,
                            t_struct.hour,
                            t_struct.minute,
                            t_struct.second);
        report_snippet.append(report_line);

        // prepare also the report file name based on the start time
        // reusing the same report_line var for preparing the time string
        base::snprintf(report_line, report_line_max_length,
                            "%4d%02d%02d%02d%02d%02d",
                            t_struct.year,
                            t_struct.month,
                            t_struct.day_of_month,
                            t_struct.hour,
                            t_struct.minute,
                            t_struct.second);
        autotest_report_filename_.append(report_line);
        autotest_report_filename_.append(kAutoTestFileNameSuffix);


        base::snprintf(report_line, report_line_max_length,
                            "<li>SiteList Length: %d</li>\n",
                            (unsigned int)autotest_links_.size());
        report_snippet.append(report_line);
        report_snippet.append("</ul>\n");
        report_snippet.append("<table border=\"1\">\n"
                "<th>No</th>"
                "<th>StartTime</th>"
                "<th>Loading time [ms]</th>"
                "<th>Shared Memory Used [KB]</th>"
                "<th>Private Memory Used [KB]</th>"
                "<th>System Memory Used [KB]</th>"
                "<th>Max Stack Used [KB]</th>"
                "<th>CPU Used [%]</th>"
                "<th>Failed [err code]</th>"
                "<th>URL</th>\n");

        // create report file
        CloseFile(OpenFile(FilePath(autotest_report_filename_), "w+"));

    } else {
        base::snprintf(report_line, report_line_max_length,
                            "<tr id=\"Line%lu\"><td>%lu</td>",
                            current_report_line_.line_,
                            current_report_line_.line_);
        report_snippet.append(report_line);

        // transform start time
        base::Time::Exploded t_struct;
        current_report_line_.start_time_.LocalExplode(&t_struct);
        base::snprintf(report_line, report_line_max_length,
                            "%4d/%02d/%02d %02d:%02d:%02d",
                            t_struct.year,
                            t_struct.month,
                            t_struct.day_of_month,
                            t_struct.hour,
                            t_struct.minute,
                            t_struct.second);
        report_snippet.append("<td>");
        report_snippet.append(report_line);
        report_snippet.append("</td>");

        // calculate time delta from start to stop in ms
        base::snprintf(report_line, report_line_max_length,
                            "<td>%lu</td>",
                (unsigned long)base::TimeDelta(
                        current_report_line_.stop_time_
                        - current_report_line_.start_time_)
                    .InMilliseconds());
        report_snippet.append(report_line);

        base::snprintf(report_line, report_line_max_length,
                            "<td>%lu</td>",
                current_report_line_.shared_mem_usage_);
        report_snippet.append(report_line);
        base::snprintf(report_line, report_line_max_length,
                            "<td>%lu</td>",
                current_report_line_.priv_mem_usage_);
        report_snippet.append(report_line);
        base::snprintf(report_line, report_line_max_length,
                            "<td>%lu</td>",
                current_report_line_.sys_mem_usage_);
        report_snippet.append(report_line);

        base::snprintf(report_line, report_line_max_length,
                            "<td>%d</td><td>%6.2f</td>",
                            0, /* TODO: add max stack used in the future*/
                            current_report_line_.cpu_usage_);
        report_snippet.append(report_line);

        // parse failed case
        if (0 != current_report_line_.error_code_) {
            base::snprintf(report_line, report_line_max_length,
                                "<td>%d</td>\n",
                                current_report_line_.error_code_);
        } else {
            base::snprintf(report_line, report_line_max_length,
                                "<td>NA</td>");
        }
        report_snippet.append(report_line);
        if (current_report_line_.url_.c_str()) {
            report_snippet.append("<td><a href=\"");
            report_snippet.append(current_report_line_.url_.c_str());
            report_snippet.append("\">");
            report_snippet.append(current_report_line_.url_.c_str());
            report_snippet.append("</a></td></tr>");
        } else {
            report_snippet.append("NONE");
        }
    }

    if (autotest_finished_) {
        report_snippet.append("</table>\n</div>\n</body>\n</html>");
    }

    if (false == base::AppendToFile(FilePath(autotest_report_filename_),
                                        report_snippet.c_str(),
                                        report_snippet.size())) {
        LOG(ERROR) << "Could not append data to report file "
                   << autotest_report_filename_;
    }

    // starting from 0 and counting up
    current_report_line_.line_++;
}

void
AcsWebContentsObserver::UpdateReportWithData(size_t priv_mem_usage,
        size_t shared_mem_usage, double cpu_usage) {
    current_report_line_.priv_mem_usage_ = priv_mem_usage;
    current_report_line_.shared_mem_usage_ = shared_mem_usage;
    current_report_line_.cpu_usage_ = cpu_usage;

    // post a task to the IO thread to write in the report file
    content::BrowserThread::GetMessageLoopProxyForThread(BrowserThread::FILE)->
                PostTask(
                        FROM_HERE,
                        base::Bind(&AcsWebContentsObserver::FinishReportUpdating,
                                            base::Unretained(this)));
}

void
AcsWebContentsObserver::StartReportUpdating() {
    PerformanceMonitorLight* pml = PerformanceMonitorLight::GetInstance();
    DCHECK(pml);

    // on the next PerformanceMonitorLight::DoTimedCollection() call
    // we should get called with memory and cpu usage data
    pml->SetCollectionCB(
            base::Bind(&AcsWebContentsObserver::UpdateReportWithData,
                    base::Unretained(this)));

    // collect current system memory usage
    current_report_line_.sys_mem_usage_ = base::GetSystemCommitCharge();
}

void
AcsWebContentsObserver::TriggerNextLoad() {
    // we have a race condition here if the timeout timer expires while
    // a DidFinishLoad() call is running, but given the low reproduciblility
    // rate and the impact we will use only a poor man's critical section
    static std::atomic_bool triggering_in_progress(false);
    bool test_val_false = false;
    const bool test_val_true = true;

    if (!triggering_in_progress.compare_exchange_strong(test_val_false, test_val_true))
        return;

    // stop the previous timeout timer as we are loading the next page
    if (timeout_timer_.IsRunning())
        timeout_timer_.Stop();

    // post a delayed task to load the next link
    if (!iterateToNextValidLink()) {
        // no more valid links and finished all loops
        LOG(INFO) << "AutoTest Finished - report stored in "
                  << autotest_report_filename_
                  << " - now quitting...";
        autotest_finished_ = true;
        StartReportUpdating();
        base::MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                             base::MessageLoop::QuitWhenIdleClosure(),
                                             base::TimeDelta::FromSeconds(10));
        return;
    }

    StartReportUpdating();

    LOG(INFO) << "AutoTest will be loading in "
              << kDefaultInterPageDelayInSeconds << "s : "
              << (*autotest_links_itr_).c_str();
    BrowserThread::PostDelayedTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&AcsWebContentsObserver::LoadNextURL,
                 base::Unretained(this)),
        TimeDelta::FromSeconds(kDefaultInterPageDelayInSeconds));

    triggering_in_progress = false;
}

void
AcsWebContentsObserver::DidFinishLoad(RenderFrameHost* render_frame_host,
                        const GURL& validated_url) {
    if (autotest_loops_ && !autotest_finished_) {
        int frame_id = render_frame_host->GetRoutingID();
        bool is_main_frame = frame_id == web_contents()->GetMainFrame()->GetRoutingID();
        if (is_main_frame) {
            LOG(INFO) << "<" << frame_id << "> AutoTest @ " << base::Time::Now().ToJavaTime()
                    << "ms Finished on : " << validated_url
                    << " is_main_frame : " << is_main_frame;
            current_report_line_.stop_time_ = base::Time::Now();
            current_report_line_.url_ = validated_url.possibly_invalid_spec();
            current_report_line_.error_code_ = 0;
            TriggerNextLoad();
        }
    }
}

void
AcsWebContentsObserver::DidFailLoad(RenderFrameHost* render_frame_host,
                        const GURL& validated_url,
                        int error_code,
                        const base::string16& error_description,
                        bool was_ignored_by_handler) {
    if (autotest_loops_ && !autotest_finished_) {
        int frame_id = render_frame_host->GetRoutingID();
        bool is_main_frame = frame_id == web_contents()->GetMainFrame()->GetRoutingID();
        if (is_main_frame) {
            LOG(INFO) << "<" << frame_id << "> AutoTest @ " << base::Time::Now().ToJavaTime() << "ms Failed   on : "
                      << validated_url << " is_main_frame : " << is_main_frame;
            current_report_line_.stop_time_ = base::Time::Now();
            current_report_line_.url_ = validated_url.possibly_invalid_spec();
            current_report_line_.error_code_ = error_code;
            TriggerNextLoad();
        }
    }
}

void
AcsWebContentsObserver::TimeoutLoad() {
    if (autotest_loops_ && !autotest_finished_) {
        GURL validated_url(*autotest_links_itr_);
        int frame_id = web_contents()->GetMainFrame()->GetRoutingID();
        LOG(INFO) << "<" << frame_id << "> AutoTest @ " << base::Time::Now().ToJavaTime() << "ms Timed out   on : "
                  << validated_url << " is_main_frame : " << true;
        current_report_line_.stop_time_ = base::Time::Now();
        current_report_line_.url_ = validated_url.possibly_invalid_spec();
        current_report_line_.error_code_ = net::ERR_TIMED_OUT;
        TriggerNextLoad();
    }
}

bool
AcsWebContentsObserver::iterateToNextValidLink() {
    if ((autotest_loop_counter_ > autotest_loops_) || (autotest_finished_) ){
        return false;
    }

    if (autotest_links_itr_ != autotest_links_.end()) {
        autotest_links_itr_++;
    }

    // jump to the beginning of the links vector when reaches the end
     if (autotest_links_itr_ == autotest_links_.end()) {
         autotest_loop_counter_++;
         if (autotest_loop_counter_ > autotest_loops_) {
             // but bail out if all the loops were executed
             return false;
         }
         autotest_links_itr_ = autotest_links_.begin();
     }

    // autotest_links_itr_ points to a valid link now
    return true;
}

}  // namespace content
