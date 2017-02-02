#include "../is/theora/ogg-encoder.hpp"

namespace is {
namespace theora {

OggEncoder::OggEncoder() {
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

OggEncoder::~OggEncoder() {
  if (context != nullptr) {
    th_encode_free(context);
  }
}

void OggEncoder::update_context() {
  if (context != nullptr) {
    th_encode_free(context);
  }

  context = th_encode_alloc(&info);
  if (context == nullptr) {
    is::log::critical("Failed to allocate context");
    exit(-1);
  }

  th_comment_init(&comment);
  flush_header();
}

void OggEncoder::flush_header() {
  header.clear();
  sent_headers = false;

  while (1) {
    auto status = th_encode_flushheader(context, &comment, &packet);
    if (status > 0) {
      header.emplace_back(make_ogg_packet(packet));
    } else if (status == 0) {
      break;
    } else {
      is::log::error("Failed to flush header");
      break;
    }
  }
}

OggPacket OggEncoder::make_ogg_packet(ogg_packet const& packet) {
  OggPacket ogg{.new_context = static_cast<bool>(packet.b_o_s),
                .data = std::vector<unsigned char>(packet.packet, packet.packet + packet.bytes),
                .packet_n = packet.packetno,
                .granule_pos = packet.granulepos};
  return ogg;
}

std::vector<OggPacket> OggEncoder::encode(cv::Mat frame) {
  std::vector<OggPacket> packets;

  const uint32_t width = frame.cols;
  const uint32_t height = frame.rows;
  if (info.pic_width != width || info.pic_height != height) {
    info.pic_width = width;
    info.pic_height = height;
    // frame width/height must be a multiple of 16, and less than 1048576
    info.frame_width = (width + 15) & ~0xF;
    info.frame_height = (height + 15) & ~0xF;
    update_context();
  }

  if (!sent_headers) {
    packets = header;
    sent_headers = true;
  }

  if (sent_headers) {
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
      is::log::warn("Failed while submitting frame");
      return packets;
    }

    if (th_encode_packetout(context, 0, &packet) <= 0) {
      is::log::warn("Failed to retrieve encoded data");
      return packets;
    }

    packets.emplace_back(make_ogg_packet(packet));
  }

  return packets;
}

std::vector<OggPacket> OggEncoder::get_header() {
  std::unique_lock<std::mutex> lock(mutex);
  return header;
}

}  // ::theora

}  // ::is