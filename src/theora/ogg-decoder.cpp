#include "../is/theora/ogg-decoder.hpp"

namespace is {
namespace theora {

OggDecoder::OggDecoder(std::vector<OggPacket> header) {
  for (auto&& packet : header) {
    decode(packet);
  }
}

boost::optional<cv::Mat> OggDecoder::decode(OggPacket& ogg) {
  ogg_packet packet;
  packet.packet = &ogg.data[0];
  packet.bytes = ogg.data.size();
  packet.packetno = ogg.packet_n;
  packet.granulepos = ogg.granule_pos;
  packet.b_o_s = ogg.new_context;

  if (ogg.new_context) {
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
        if (status > 0) {
          is::log::info("Header successfully processed");
        } else if (status == 0) {
          is::log::info("First video data packet received");
          context = th_decode_alloc(&info, setup);
          state = WAITING_FIRST_KEYFRAME;
          break;
        } else {
          is::log::error("Error processing header");
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
          is::log::error("Failed to decode frame");
          return boost::none;
        }

        th_ycbcr_buffer buffer;
        if (th_decode_ycbcr_out(context, buffer)) {
          is::log::error("Failed to decode buffer");
          return boost::none;
        }

        std::vector<cv::Mat> planes(3);
        for (size_t i = 0; i < 3; i++) {
          planes[i] = cv::Mat(buffer[i].height, buffer[i].width, CV_8UC1, buffer[i].data, buffer[i].stride);
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

OggDecoder::~OggDecoder() {
  if (setup != nullptr) {
    th_setup_free(setup);
  }
  if (context != nullptr) {
    th_decode_free(context);
  }
}

}  //::theora
}  //::is