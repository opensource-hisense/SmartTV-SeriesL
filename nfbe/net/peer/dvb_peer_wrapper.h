// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef NET_PEER_DVB_PEER_WRAPPER_H_
#define NET_PEER_DVB_PEER_WRAPPER_H_

#include <string>
#include <vector>
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "net/base/net_export.h"
#include "net/http/http_byte_range.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job.h"
#include "net/url_request/url_request_dvb_job.h"

namespace net {
/*!
 *  \brief <h2>Open a DVB stream for reading.</h2>
 *  \param in_fname		FilePath containing the dvb:// uri INCLUDING the 'dvb://' part
 *  \param in_flag		mode in which dvb stream has to open.
 *
 *  \return success Return a dvb stream descriptor integer >=0 if open succeeded.
 *   Otherwise return -1.
 */
int DVBOpenPeerWrapper(const base::FilePath& in_fname, const char* in_flag);

/*!
 *	\brief <h2>Get the file size of the dvb stream.</h2>
 *	\param dvb_stream_descriptor	dvb stream descriptor whose this API returns size
 *
 *  \note This peer function might be called on a different thread than
 *	      the DVBReadPeer() or DVBClosePeer() below.
 *
 *	\return file size of this dvb stream descriptor
 */
size_t DVBGetFileSizePeerWrapper(int dvb_stream_descriptor);

/*!
 *	\brief <h2>Get the mime type of the dvb stream.</h2>
 *	\param dvb_stream_descriptor	dvb stream descriptor whose this API returns mime type
 *  \param mime_type set mime type if this dvb stream has mime type
 *
 *  \note This peer function might be called on a different thread than
 *	      the DVBReadPeer() or DVBClosePeer() below.
 *
 *	\return true if this dvb stream has mime type and false otherwise.
 */
bool DVBGetMimeTypePeerWrapper(int dvb_stream_descriptor,
                               std::string& mime_type);

 /*!
 *  \brief <h2>Read from a DVB stream.</h2>
 *  \param dvb_stream_descriptor		dvb stream descriptor to be read
 *  \param out_buf		output buffer where the bytes read must be stored
 *  \param in_len		maximum number of bytes to be read from the DVB stream
 *
 *  \return success Return an integer >=0 i.e. the number of bytes read from the DVB stream.
 *   failure Return -1
 */
int DVBReadPeerWrapper(int dvb_stream_descriptor, void *out_buf, int in_len);

 /*!
 *  \brief <h2>Close a given DVB stream.</h2>
 *  \param dvb_stream_descriptor dvb stream descriptor which should be closed.
 */
void DVBClosePeerWrapper(int dvb_stream_descriptor);

}  // end namespace net
#endif  // NET_PEER_DVB_PEER_WRAPPER_H_
