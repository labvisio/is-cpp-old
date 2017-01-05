#ifndef __IS_DRIVER_WEBCAM_HPP__
#define __IS_DRIVER_WEBCAM_HPP__

#include <mutex>
#include <opencv2/videoio.hpp>
#include <thread>
#include "../msgs/camera.hpp"

namespace is {
namespace driver {

using namespace std::chrono_literals;
using namespace is::msg::camera;

struct Webcam {
  cv::VideoCapture webcam;
  std::mutex mutex;
  std::chrono::time_point<std::chrono::system_clock> last_frame_time_point;

  Webcam() : webcam(0) { assert(webcam.isOpened()); }

  void set_fps(double fps) {
    std::lock_guard<std::mutex> lock(mutex);
    webcam.set(cv::CAP_PROP_FPS, fps);
  }

  void set_resolution(Resolution resolution) {
    std::lock_guard<std::mutex> lock(mutex);
    webcam.set(cv::CAP_PROP_FRAME_WIDTH, resolution.width);
    webcam.set(cv::CAP_PROP_FRAME_HEIGHT, resolution.height);
  }

  double get_fps() {
    std::lock_guard<std::mutex> lock(mutex);
    return webcam.get(cv::CAP_PROP_FPS);
  }

  double get_max_fps() { return 30.0; }

  ImageType get_image_type() { return ImageType::RGB; }

  Resolution get_resolution() {
    std::lock_guard<std::mutex> lock(mutex);
    Resolution resolution;
    resolution.width = webcam.get(cv::CAP_PROP_FRAME_WIDTH);
    resolution.height = webcam.get(cv::CAP_PROP_FRAME_HEIGHT);
    return resolution;
  }

  cv::Mat get_frame() {
    cv::Mat frame;
    std::lock_guard<std::mutex> lock(mutex);
    webcam >> frame;
    return frame;
  }
};

}  // ::driver
}  // ::is

#endif  // __IS_DRIVER_WEBCAM_HPP__
