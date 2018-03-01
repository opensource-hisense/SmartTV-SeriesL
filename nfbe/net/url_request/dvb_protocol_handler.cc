// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "net/url_request/dvb_protocol_handler.h"

#include "base/logging.h"
#include "base/task_runner.h"
#include "net/base/filename_util.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_error_job.h"
#include "net/url_request/url_request_file_dir_job.h"
#include "net/url_request/url_request_dvb_job.h"

namespace net {

DvbProtocolHandler::DvbProtocolHandler(
    const scoped_refptr<base::TaskRunner>& dvb_task_runner)
    : dvb_task_runner_(dvb_task_runner) {}

DvbProtocolHandler::~DvbProtocolHandler() {}

URLRequestJob* DvbProtocolHandler::MaybeCreateJob(
    URLRequest* request, NetworkDelegate* network_delegate) const {
  base::FilePath file_path(request->url().spec());

  // TODO: check with middleware if we are allowed to access this resource
  // if not return new URLRequestErroJob(request, network_delegate, ERR_ACCESS_DENIED);
  if (!network_delegate ||
      !network_delegate->CanAccessFile(*request, file_path)) {
    return new URLRequestErrorJob(request, network_delegate, ERR_ACCESS_DENIED);
  }

  // Use a regular file request job for all non-directories (including invalid
  // file names).
  return new URLRequestDvbJob(request, network_delegate, file_path, dvb_task_runner_);
}

bool DvbProtocolHandler::IsSafeRedirectTarget(const GURL& location) const {
  return false;
}

}  // namespace net
