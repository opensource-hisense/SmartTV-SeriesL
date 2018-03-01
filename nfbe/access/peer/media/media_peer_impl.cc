// Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.

#include "access/peer/include/media/media_peer.h"

namespace peer {

namespace media {

// Dummy implementation

BE_PEER_API bool MediaIsSupportedMediaSourceTypePeer(
    const std::string& type, const std::vector<std::string>& codecs,
    unsigned width, unsigned height, unsigned framerate,
    unsigned bitrate, unsigned channels, const std::string& eotf,
    enum ParserType *parser_type, bool *has_audio, bool *has_video) {

  // Format parameters
  fprintf(stderr, "%s", type.c_str());
  if (!codecs.empty()) {
    fprintf(stderr, "; codecs=\"");
    for (auto it = codecs.begin(); it != codecs.end(); ++it) {
      fprintf(stderr, "%s", it->c_str());  // TODO: comma
    }
    fprintf(stderr, "\"");
  }
  if (width != 0) {
    fprintf(stderr, "; width=%u", width);
  }
  if (height != 0) {
    fprintf(stderr, "; height=%u", height);
  }
  if (framerate != 0) {
    fprintf(stderr, "; framerate=%u", framerate);
  }
  if (bitrate != 0) {
    fprintf(stderr, "; bitrate=%u", bitrate);
  }
  if (channels != 0) {
    fprintf(stderr, "; channels=%u", channels);
  }
  if (!eotf.empty()) {
    fprintf(stderr, "; eotf=%s", eotf.c_str());
  }

  // To pass Youtube's sanity check
  if (width > 4096) {
    return false;
  }
  if (height > 2160) {
    return false;
  }
  if (framerate > 60) {
    return false;
  }
  if (bitrate >= 2000000000) {
    return false;
  }
  if (channels > 10) {
    return false;
  }

  if (has_audio) {
    *has_audio = (type == "audio/webm" || type == "audio/mp4");
  }

  if (has_video) {
    *has_video = (type == "video/webm" || type == "video/mp4");
  }

  if (parser_type) {
    if (type == "audio/webm" || type == "video/webm") {
      *parser_type = WEBM_PARSER;
    } else if (type == "audio/mp4" || type == "video/webm") {
      *parser_type = MP4_PARSER;
    } else {
      *parser_type = NO_PARSER;
    }
  }

  return true;
}

}  // namespace media

}  // namespace peer
