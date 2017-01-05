#ifndef __IS_MSG_CAMERA_HPP__
#define __IS_MSG_CAMERA_HPP__

#include <opencv2/imgcodecs.hpp>
#include <string>
#include <vector>
#include "../packer.hpp"

namespace is {
namespace msg {
namespace camera {

struct CompressedImage {
  std::string format;               // Image format: ".png", ".jpg"
  std::vector<unsigned char> data;  // Image binary data

  MSGPACK_DEFINE_ARRAY(format, data);
};

struct RegionOfInterest {
  int x_offset;  // Leftmost pixel of the ROI
  int y_offset;  // Topmost pixel of the ROI
  int height;    // Height of ROI
  int width;     // Width of ROI

  MSGPACK_DEFINE_ARRAY(x_offset, y_offset, height, width);
};

struct Resolution {
  int height;
  int width;

  MSGPACK_DEFINE_ARRAY(height, width);
};

enum class ImageType { RGB, GRAY };

}  // ::camera
}  // ::msg
}  // ::is

// Enum packing must be done on global namespace
MSGPACK_ADD_ENUM(is::msg::camera::ImageType);

#endif  // __IS_MSG_CAMERA_HPP__
