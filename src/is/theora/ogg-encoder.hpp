#ifndef __IS_THEORA_OGG_ENCODER_HPP__
#define __IS_THEORA_OGG_ENCODER_HPP__

#include <theora/codec.h>
#include <theora/theoradec.h>
#include <theora/theoraenc.h>

#include <opencv2/imgproc.hpp>

#include <mutex>

#include "../logger.hpp"
#include "../msgs/camera.hpp"

namespace is {

namespace theora {

using OggPacket = is::msg::camera::OggPacket; 

struct OggEncoder {
  th_info info;
  th_comment comment;
  th_enc_ctx* context = nullptr;

  ogg_packet packet;

  std::mutex mutex;
  std::vector<OggPacket> header;
  bool sent_headers = false;

  void update_context();
  void flush_header();
  OggPacket make_ogg_packet(ogg_packet const& packet);

 public:
  OggEncoder();
  virtual ~OggEncoder();

  std::vector<OggPacket> get_header();
  std::vector<OggPacket> encode(cv::Mat frame);
};

}  // ::theora

}  // ::is

#endif  // __IS_THEORA_OGG_ENCODER_HPP__