#ifndef __IS_MSG_CAMERA_HPP__
#define __IS_MSG_CAMERA_HPP__

#include <boost/optional.hpp>
#include "../packer.hpp"
#include "common.hpp"

namespace is {
namespace msg {
namespace camera {

using namespace is::msg::common;

struct TheoraPacket {
  bool new_header;
  std::vector<unsigned char> data;  // Packet binary data
  IS_DEFINE_MSG(new_header, data);
};

struct CompressedImage {
  std::string format;               // Image format: ".png", ".jpg"
  std::vector<unsigned char> data;  // Image binary data
  IS_DEFINE_MSG(format, data);
};

struct RegionOfInterest {
  unsigned int x_offset;  // Leftmost pixel of the ROI
  unsigned int y_offset;  // Topmost pixel of the ROI
  unsigned int height;    // Height of ROI
  unsigned int width;     // Width of ROI
  IS_DEFINE_MSG(x_offset, y_offset, height, width);
};

struct Resolution {
  unsigned int height;
  unsigned int width;
  IS_DEFINE_MSG(height, width);
};

struct ImageType {
  std::string value;
  IS_DEFINE_MSG(value);
};

namespace image_type {
const ImageType rgb{"rgb"};
const ImageType gray{"gray"};
}

struct Exposure {
  float value;
  boost::optional<bool> auto_mode;
  IS_DEFINE_MSG(value, auto_mode);
};

struct Shutter {
  boost::optional<float> percent;
  boost::optional<float> ms;
  boost::optional<bool> auto_mode;
  IS_DEFINE_MSG(percent, ms, auto_mode);
};

struct Gain {
  boost::optional<float> percent;
  boost::optional<float> db;
  boost::optional<bool> auto_mode;
  IS_DEFINE_MSG(percent, db, auto_mode);
};

struct WhiteBalance {
  boost::optional<unsigned int> red;
  boost::optional<unsigned int> blue;
  boost::optional<bool> auto_mode;
  IS_DEFINE_MSG(red, blue, auto_mode);
};

struct Configuration {
  boost::optional<Resolution> resolution;
  boost::optional<SamplingRate> sampling_rate;
  boost::optional<ImageType> image_type;
  boost::optional<float> brightness;
  boost::optional<Exposure> exposure;
  boost::optional<Shutter> shutter;
  boost::optional<Gain> gain;
  boost::optional<WhiteBalance> white_balance;
  IS_DEFINE_MSG(resolution, sampling_rate, image_type, brightness, exposure, shutter, gain, white_balance);
};

}  // ::camera
}  // ::msg
}  // ::is

#endif  // __IS_MSG_CAMERA_HPP__
