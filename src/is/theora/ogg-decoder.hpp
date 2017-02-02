#ifndef __IS_THEORA_OGG_DECODER_HPP__
#define __IS_THEORA_OGG_DECODER_HPP__

#include <theora/codec.h>
#include <theora/theoradec.h>
#include <theora/theoraenc.h>

#include <opencv2/imgproc.hpp>

#include <boost/optional.hpp>

#include "../logger.hpp"
#include "../msgs/camera.hpp"

namespace is {

namespace theora {

using OggPacket = is::msg::camera::OggPacket; 

struct OggDecoder {
  th_info info;
  th_comment comment;
  th_setup_info* setup = nullptr;
  th_dec_ctx* context = nullptr;

  enum State { NEW_CONTEXT, RECEIVING_HEADER, WAITING_FIRST_KEYFRAME, RECEIVING_PACKETS };
  State state = NEW_CONTEXT;

  OggDecoder(std::vector<OggPacket> header = {});
  virtual ~OggDecoder();

  boost::optional<cv::Mat> decode(OggPacket& ogg);

};

}  // ::theora

}  // ::is

#endif  // __IS_THEORA_OGG_DECODER_HPP__