#ifndef __IS_THEORA_DECODER_HPP__
#define __IS_THEORA_DECODER_HPP__

#include <theora/codec.h>
#include <theora/theoradec.h>
#include <theora/theoraenc.h>
#include <boost/optional.hpp>
#include <opencv2/imgproc.hpp>

#include "logger.hpp"
#include "msgs/camera.hpp"

namespace is {

using TheoraPacket = is::msg::camera::TheoraPacket;

struct TheoraDecoder {
  th_info info;
  th_comment comment;
  th_setup_info* setup = nullptr;
  th_dec_ctx* context = nullptr;

  enum State { NEW_CONTEXT, RECEIVING_HEADER, WAITING_FIRST_KEYFRAME, RECEIVING_PACKETS };
  State state = NEW_CONTEXT;

  void set_headers(std::vector<TheoraPacket> const& headers) { 
    for (auto&& header : headers) {
      decode(header);
    }
  }
    
  boost::optional<cv::Mat> decode(TheoraPacket const& p) {
    ogg_packet packet;
    packet.packet = (unsigned char*) &p.data[0];  // :{
    packet.bytes = p.data.size();
    packet.b_o_s = p.new_header;

    if (packet.b_o_s) {
      state = NEW_CONTEXT;
    }

    while (1) {
      switch (state) {
        case NEW_CONTEXT: {
          if (context != nullptr) {
            th_info_clear(&info);
            th_comment_clear(&comment);
            th_setup_free(setup);
            th_decode_free(context);
          }
          th_info_init(&info);
          th_comment_init(&comment);
          setup = nullptr;
          context = nullptr;
          state = RECEIVING_HEADER;
          break;
        }

        case RECEIVING_HEADER: {
          int status = th_decode_headerin(&info, &comment, &setup, &packet);
          log::info("status={} len={}", status, packet.bytes);
          if (status > 0) {
            log::info("Header successfully processed");
          } else if (status == 0) {
            log::info("First video data packet received");
            context = th_decode_alloc(&info, setup);
            state = WAITING_FIRST_KEYFRAME;
            break;
          } else {
            log::error("Error processing header");
          }
          return boost::none;
        }

        case WAITING_FIRST_KEYFRAME: {
          if (th_packet_iskeyframe(&packet) == 1) {
            state = RECEIVING_PACKETS;
            break;
          }
          return boost::none;
        }

        case RECEIVING_PACKETS: {
          if (th_decode_packetin(context, &packet, nullptr)) {
            log::error("Failed to decode frame");
            return boost::none;
          }

          th_ycbcr_buffer buffer;
          if (th_decode_ycbcr_out(context, buffer)) {
            log::error("Failed to decode buffer");
            return boost::none;
          }

          std::vector<cv::Mat> planes(3);
          for (size_t i = 0; i < 3; i++) {
            planes[i] = cv::Mat(buffer[i].height, buffer[i].width, CV_8UC1, buffer[i].data,
                                buffer[i].stride);
          }
          pyrUp(planes[1], planes[1]);
          pyrUp(planes[2], planes[2]);

          cv::Mat frame;
          cv::merge(planes, frame);
          cv::cvtColor(frame, frame, CV_YCrCb2BGR);

          return frame;
        }
      }
    }
  }

  ~TheoraDecoder() {
    if (setup != nullptr) {
      th_setup_free(setup);
    }
    if (context != nullptr) {
      th_decode_free(context);
    }
  }

};  // ::TheoraDecoder

}  // ::is

#endif  // __IS_THEORA_DECODER_HPP__