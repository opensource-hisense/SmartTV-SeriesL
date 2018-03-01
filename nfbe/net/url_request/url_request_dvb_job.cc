// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "net/url_request/url_request_dvb_job.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"

namespace net {

URLRequestDvbJob::FileMetaInfo::FileMetaInfo()
    : file_size(0),
      mime_type_result(false),
      dvb_stream_descriptor(-1) {
}

URLRequestDvbJob::URLRequestDvbJob(
    URLRequest* request,
    NetworkDelegate* network_delegate,
    const base::FilePath& file_path,
    const scoped_refptr<base::TaskRunner>& dsmcc_task_runner)
    : URLRequestJob(request, network_delegate),
      file_path_(file_path),
      dsmcc_task_runner_(dsmcc_task_runner),
      remaining_bytes_(0),
      weak_ptr_factory_(this) {
      dvb_stream_descriptor_ = -1;
}

void URLRequestDvbJob::Start() {
  FileMetaInfo* meta_info = new FileMetaInfo();
  // Don't try to optimize this running on a separate thread because
  // you will meet problems(read crashes) when responding to the
  // resource loader from the same thread from which it calls.
  dsmcc_task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&URLRequestDvbJob::OpenDvbStream,
                 file_path_,
                 base::Unretained(meta_info)),
      base::Bind(&URLRequestDvbJob::DidOpenDvbStream,
                 weak_ptr_factory_.GetWeakPtr(),
                 base::Owned(meta_info)));
}

void URLRequestDvbJob::Kill() {
  weak_ptr_factory_.InvalidateWeakPtrs();

  URLRequestJob::Kill();
}

// static
void URLRequestDvbJob::OpenDvbStream(const base::FilePath& file_path,
                                     FileMetaInfo* meta_info) {
  meta_info->dvb_stream_descriptor = DVBOpenPeerWrapper(file_path, "rb+");
  if (meta_info->dvb_stream_descriptor == -1)
    return;
  meta_info->file_size =
      DVBGetFileSizePeerWrapper(meta_info->dvb_stream_descriptor);
  meta_info->mime_type_result =
      DVBGetMimeTypePeerWrapper(meta_info->dvb_stream_descriptor,
                                meta_info->mime_type);
}

void URLRequestDvbJob::DidOpenDvbStream(const FileMetaInfo* meta_info) {
  if (meta_info->dvb_stream_descriptor  == -1) {
    LOG(INFO) << "URLRequestDvbJob::OpenDvbStream DVBOpen Failed.";
    NotifyStartError(
            URLRequestStatus(
                    URLRequestStatus::FAILED,
                    net::ERR_FILE_NOT_FOUND));
    return;
  }

  dvb_stream_descriptor_ = meta_info->dvb_stream_descriptor;
  meta_info_ = *meta_info;
  remaining_bytes_ = meta_info_.file_size;

  set_expected_content_size(remaining_bytes_);
  NotifyHeadersComplete();
}

int URLRequestDvbJob::ReadRawData(IOBuffer* dest,
                                    int dest_size) {
  DCHECK_NE(dest_size, 0);
  DCHECK_GE(remaining_bytes_, 0);

  if (remaining_bytes_ <= 0) {
    DVBClosePeerWrapper(dvb_stream_descriptor_);
    return 0;
  }

  if (!dest->data())
    return 0;

  // memset dest buffer to 0
  memset(dest->data(), 0, dest_size);

  if (remaining_bytes_ < dest_size)
    dest_size = static_cast<int>(remaining_bytes_);

  int bytes_read  = DVBReadPeerWrapper(dvb_stream_descriptor_,
                                       reinterpret_cast<void*>(dest->data()),
                                       dest_size);
  if (bytes_read == -1) {
    DVBClosePeerWrapper(dvb_stream_descriptor_);
    return bytes_read;
  }

  remaining_bytes_ -= bytes_read;
  set_expected_content_size(remaining_bytes_);
  return bytes_read;
}

bool URLRequestDvbJob::IsRedirectResponse(GURL* location,
                                           int* http_status_code) {
  return false;
}

bool URLRequestDvbJob::GetMimeType(std::string* mime_type) const {
  DCHECK(request_);
  if (meta_info_.mime_type_result) {
    *mime_type = meta_info_.mime_type;
    return true;
  }
  return false;
}

URLRequestDvbJob::~URLRequestDvbJob() {
  DVBClosePeerWrapper(dvb_stream_descriptor_);
}

}  // namespace net
