// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef NET_URL_REQUEST_URL_REQUEST_DVB_JOB_H_
#define NET_URL_REQUEST_URL_REQUEST_DVB_JOB_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "net/base/net_export.h"
#include "net/http/http_byte_range.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job.h"
#include "net/peer/dvb_peer_wrapper.h"

namespace base {
class TaskRunner;
}

namespace net {

// A request job that handles reading file URLs
class NET_EXPORT URLRequestDvbJob : public URLRequestJob {
 public:
  URLRequestDvbJob(URLRequest* request,
                    NetworkDelegate* network_delegate,
                    const base::FilePath& file_path,
                    const scoped_refptr<base::TaskRunner>& dsmcc_task_runner);

  // URLRequestJob:
  void Start() override;
  void Kill() override;
  int ReadRawData(IOBuffer* buf, int buf_size) override;
  bool IsRedirectResponse(GURL* location, int* http_status_code) override;
  bool GetMimeType(std::string* mime_type) const override;

 protected:
  ~URLRequestDvbJob() override;

  int64_t remaining_bytes() const { return remaining_bytes_; }

  // The OS-specific full path name of the file
  base::FilePath file_path_;

 private:
  // Meta information about the file. It's used as a member in the
  // URLRequestDvbJob and also passed between threads because disk access is
  // necessary to obtain it.
  struct FileMetaInfo {
    FileMetaInfo();

    // Size of the file.
    int64_t file_size;
    // Mime type associated with the file.
    std::string mime_type;
    // Result returned from GetMimeTypeFromFile(), i.e. flag showing whether
    // obtaining of the mime type was successful.
    bool mime_type_result;

    // The file descriptor for opened dvb stream by dvb peer.
    // This is passed from OpenDvbStream to the actual URLRequestDvbJob
    // instance by storing it here and calling DidOpenDvbStream()
    // to copy it from here.
    int dvb_stream_descriptor;

    //FIXME: This struct must be kept in sync with dvb_peer.h !!!
    // Use the same FileMetaInfo definition both in peer and here.
  };

  // Open the DVB stream and fetches stream meta info
  static void OpenDvbStream(const base::FilePath& file_path,
                            FileMetaInfo* meta_info);

  // Callback after fetching file info on a background thread.
  void DidOpenDvbStream(const FileMetaInfo* meta_info);

  FileMetaInfo meta_info_;
  const scoped_refptr<base::TaskRunner> dsmcc_task_runner_;

  int64_t remaining_bytes_;

  //file descriptor for opened dvb stream by dvb peer
  int dvb_stream_descriptor_;

  base::WeakPtrFactory<URLRequestDvbJob> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestDvbJob);
};

}  // namespace net

#endif  // NET_URL_REQUEST_URL_REQUEST_DVB_JOB_H_
