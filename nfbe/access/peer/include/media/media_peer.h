// Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.

#ifndef ACCESS_PEER_MEDIA_MEDIA_PEER_H_
#define ACCESS_PEER_MEDIA_MEDIA_PEER_H_

#include <string>
#include <vector>

#include "access/peer/include/common/be_config.h"

namespace peer {

namespace media {

/*!
 * \enum ParserType Type of Media Parser
 */
enum ParserType {
  WEBM_PARSER /*! WebM Parser */,
  ADTS_PARSER /*! ADTS Parser */,
  MP3_PARSER /*! MP3 Parser */,
  MP4_PARSER /*! MP4 Parser */,
  MP2T_PARSER /*! MP2T Parser */,
  NO_PARSER /*! Parser not found */,
};

/*!
 * Query media is supported.
 * \param type MIMEType.
 * \param codecs List of codecs.
 * \param width The x-axis video playback resolution in pixels.
                Ignore if it is 0.
 * \param height The y-axis video playback resolution in pixels.
                 Ignore if it is 0.
 * \param framerate The video playback frame rate in frames per second.
                    Ignore if it is 0.
 * \param bitrate The playback bitrate in bits per second.
                  Ignore if it is 0.
 * \param channels The number of absolute audio channels.
                   Ignore if it is 0.
 * \param eotf The supported electro-optic transfer function.
               See Media TV HTML5 Technical Requirements 2017 Section 16.3.
               Ignore if it is empty string.
 * \param parser_type If non-null return parser type for this media.
 * \param has_audio If non-null return this media includes audio.
 * \param has_video If non-null return this media includes video.
 * \return Whether the media with specified parameters is supported or not.
 * If parser_type, has_audio or has_video are non-null they also return value.
 * \warning This function is always called from Render Process.
 */
BE_PEER_API bool MediaIsSupportedMediaSourceTypePeer(
    const std::string& type, const std::vector<std::string>& codecs,
    unsigned width, unsigned height, unsigned framerate,
    unsigned bitrate, unsigned channels, const std::string& eotf,
    enum ParserType *parser_type, bool *has_audio, bool *has_video);

}  // namespace media

}  // namespace peer

#endif /* ACCESS_PEER_MEDIA_MEDIA_PEER_H_ */
