// Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#include <string>

#include "net/peer/dvb_peer_wrapper.h"
#if defined(HBBTV_ON)
#include "access/peer/include/dvb/dvb_peer.h"
#endif
#include "base/trace_event/trace_event.h"

namespace net {

int DVBOpenPeerWrapper(const base::FilePath& in_fname, const char* in_flag) {
  TRACE_EVENT0("file", "DVBOpenPeerWrapper()");
#if defined(HBBTV_ON)
  // FIXME: this does not work on Windows where value() returns
  // std::wstring instead of std::string.
  int dvb_fd = peer::dvb::DVBOpenPeer(in_fname.value(), in_flag);
  return dvb_fd;
#else
  return -1;
#endif
}

int DVBReadPeerWrapper(int dvb_stream_descriptor, void *out_buf, int in_len) {
  TRACE_EVENT0("file", "DVBReadPeerWrapper()");
#if defined(HBBTV_ON)
  int bytes_read =
      peer::dvb::DVBReadPeer(dvb_stream_descriptor, out_buf, in_len);
  return bytes_read;
#else
  return 0;
#endif
}

size_t DVBGetFileSizePeerWrapper(int dvb_stream_descriptor) {
  TRACE_EVENT0("file", "DVBGetFileSizePeerWrapper()");
#if defined(HBBTV_ON)
  return peer::dvb::DVBGetFileSizePeer(dvb_stream_descriptor);
#else
  return 0;
#endif
}

bool DVBGetMimeTypePeerWrapper(int dvb_stream_descriptor,
                               std::string& mime_type) {
  TRACE_EVENT0("file", "DVBGetMimeTypePeerWrapper()");
#if defined(HBBTV_ON)
  return peer::dvb::DVBGetMimeTypePeer(dvb_stream_descriptor, mime_type);
#else
  return false;
#endif
}

void DVBClosePeerWrapper(int dvb_stream_descriptor) {
  TRACE_EVENT0("file", "DVBClosePeerWrapper()");
#if defined(HBBTV_ON)
  peer::dvb::DVBClosePeer(dvb_stream_descriptor);
#endif
  return;
}

}  // end namespace net
