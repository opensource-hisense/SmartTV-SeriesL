/*
 *  Copyright (c) 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/media/base/codec.h"

// This code is copied from the webrtc/media/base/codec.cc
// These functions are here to prevent undefined symbols
// on some platforms like ARM.A15

#include <algorithm>
#include <sstream>

namespace cricket {

const int kMaxPayloadId = 127;

std::string AudioCodec::ToString() const {
  std::ostringstream os;
  os << "AudioCodec[" << id << ":" << name << ":" << clockrate << ":" << bitrate
     << ":" << channels << ":" << preference << "]";
  return os.str();
}

std::string VideoCodec::ToString() const {
  std::ostringstream os;
  os << "VideoCodec[" << id << ":" << name << ":" << width << ":" << height
     << ":" << framerate << ":" << preference << "]";
  return os.str();
}

std::string DataCodec::ToString() const {
  std::ostringstream os;
  os << "DataCodec[" << id << ":" << name << "]";
  return os.str();
}

}  // namespace cricket
