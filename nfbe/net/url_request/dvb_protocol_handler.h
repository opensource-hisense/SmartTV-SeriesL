// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef NET_URL_REQUEST_DVB_PROTOCOL_HANDLER_H_
#define NET_URL_REQUEST_DVB_PROTOCOL_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_request_job_factory.h"

class GURL;

namespace base {
class TaskRunner;
}

namespace net {

class NetworkDelegate;
class URLRequestJob;

// Implements a ProtocolHandler for dvb jobs. If |network_delegate_| is NULL,
// then all file requests will fail with ERR_ACCESS_DENIED.
class NET_EXPORT DvbProtocolHandler :
    public URLRequestJobFactory::ProtocolHandler {
 public:
  explicit DvbProtocolHandler(
      const scoped_refptr<base::TaskRunner>& file_task_runner);
  ~DvbProtocolHandler() override;
  URLRequestJob* MaybeCreateJob(
      URLRequest* request,
      NetworkDelegate* network_delegate) const override;
  bool IsSafeRedirectTarget(const GURL& location) const override;

 private:
  const scoped_refptr<base::TaskRunner> dvb_task_runner_;
  DISALLOW_COPY_AND_ASSIGN(DvbProtocolHandler);
};

}  // namespace net

#endif  // NET_URL_REQUEST_DVB_PROTOCOL_HANDLER_H_
