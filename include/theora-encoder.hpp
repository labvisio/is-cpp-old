#ifndef __IS_THEORA_OGG_ENCODER_HPP__
#define __IS_THEORA_OGG_ENCODER_HPP__

#include <theora/codec.h>
#include <theora/theoradec.h>
#include <theora/theoraenc.h>
#include <chrono>
#include <opencv2/imgproc.hpp>
#include <string>
#include <vector>
#include <mutex>

#include "logger.hpp"
#include "msgs/camera.hpp"

namespace is {

using TheoraPacket = is::msg::camera::TheoraPacket;

struct TheoraEncoder {
  th_info info;
  th_comment comment;
  th_enc_ctx* context = nullptr;
  std::vector<TheoraPacket> headers;
  std::mutex mutex;

  TheoraEncoder() {
    th_info_init(&info);
    info.pic_x = 0;
    info.pic_y = 0;
    info.pic_width = 0;
    info.pic_height = 0;
    info.frame_width = 0;
    info.frame_height = 0;
    info.colorspace = TH_CS_UNSPECIFIED;
    info.pixel_fmt = TH_PF_420;
    info.aspect_numerator = 1;
    info.aspect_denominator = 1;
    info.fps_numerator = 1;
    info.fps_denominator = 1;
    info.keyframe_granule_shift = 6;
    info.target_bitrate = 4 * 1024 * 8;
  }

  ~TheoraEncoder() {
    if (context != nullptr) {
      th_encode_free(context);
    }
  }

  void update_context() {
    if (context != nullptr) {
      th_encode_free(context);
    }
    context = th_encode_alloc(&info);
    if (context == nullptr) {
      log::critical("Failed to allocate context");
    }
    th_comment_init(&comment);
  }

  void flush_headers() {
    std::unique_lock<std::mutex> lock(mutex);
    headers.clear();

    while (1) {
      ogg_packet packet;
      auto status = th_encode_flushheader(context, &comment, &packet);
      switch (status) {
        case TH_EFAULT:
        case TH_EBADHEADER:
        case TH_EVERSION:
        case TH_ENOTFORMAT:
          log::error("Failed to flush header");
          return;
        case 0:
          log::info("Done flushing header");
          return;
        default:
          log::info("Flushing header");
          TheoraPacket p;
          p.data = std::vector<unsigned char>(packet.packet, packet.packet + packet.bytes);
          p.new_header = packet.b_o_s;
          headers.emplace_back(p);
      }
    }
  }

  std::vector<TheoraPacket> get_headers() {
    std::unique_lock<std::mutex> lock(mutex);
    return headers;
  }

  void set_dimensions(uint32_t width, uint32_t height) {
    info.pic_width = width;
    info.pic_height = height;
    // frame width/height must be a multiple of 16, and less than 1048576
    info.frame_width = (width + 15) & ~0xF;
    info.frame_height = (height + 15) & ~0xF;
  }

  std::vector<TheoraPacket> encode(cv::Mat frame) {
    std::vector<TheoraPacket> packets;

    const uint32_t width = frame.cols;
    const uint32_t height = frame.rows;
    if (info.pic_width != width || info.pic_height != height) {
      set_dimensions(width, height);
      update_context();
      flush_headers();
      packets = headers;
    }

    cv::cvtColor(frame, frame, CV_BGR2YCrCb);
    cv::Mat planes[3];
    split(frame, planes);
    cv::pyrDown(planes[1], planes[1]);
    cv::pyrDown(planes[2], planes[2]);

    th_ycbcr_buffer buffer;
    for (size_t i = 0; i < 3; i++) {
      buffer[i].width = planes[i].cols;
      buffer[i].height = planes[i].rows;
      buffer[i].stride = planes[i].step;
      buffer[i].data = planes[i].data;
    }

    if (th_encode_ycbcr_in(context, buffer)) {
      log::warn("Failed while submitting frame");
      return packets;
    }

    ogg_packet packet;
    if (th_encode_packetout(context, 0, &packet) <= 0) {
      log::warn("Failed to retrieve encoded data");
      return packets;
    }

    TheoraPacket p;
    p.data = std::vector<unsigned char>(packet.packet, packet.packet + packet.bytes);
    p.new_header = packet.b_o_s;
    packets.emplace_back(p);

    return packets;
  }

};  // TheoraEncoder

}  // ::is

#endif  // __IS_THEORA_OGG_ENCODER_HPP__