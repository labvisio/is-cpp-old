#ifndef __IS_DRIVER_WEBCAM_HPP__
#define __IS_DRIVER_WEBCAM_HPP__

#include <mutex>
#include <opencv2/videoio.hpp>
#include <thread>
#include "../msgs/camera.hpp"
#include "../msgs/common.hpp"

namespace is {
namespace driver {

using namespace std::chrono_literals;
using namespace is::msg::camera;
using namespace is::msg::common;

struct Webcam {
  cv::VideoCapture webcam;
  std::mutex mutex;
  TimeStamp last_timestamp;

  Webcam() : webcam(0) { assert(webcam.isOpened()); }

  void set_sample_rate(SamplingRate sample_rate) {
    std::lock_guard<std::mutex> lock(mutex);
    double fps ;
    if (sample_rate.rate == 0.0 && sample_rate.period > 0) {
      fps = static_cast<double>(1000/sample_rate.period);
      webcam.set(cv::CAP_PROP_FPS, fps);
    } else if (sample_rate.rate > 0.0 && sample_rate.period == 0) {
      fps = sample_rate.rate;
      webcam.set(cv::CAP_PROP_FPS, fps);
    }
  }

  void set_resolution(Resolution resolution) {
    std::lock_guard<std::mutex> lock(mutex);
    webcam.set(cv::CAP_PROP_FRAME_WIDTH, resolution.width);
    webcam.set(cv::CAP_PROP_FRAME_HEIGHT, resolution.height);
  }

  SamplingRate get_sample_rate() {
    std::lock_guard<std::mutex> lock(mutex);
    return {webcam.get(cv::CAP_PROP_FPS)};
  }

  double get_max_fps() { return 30.0; }

  void set_image_type(ImageType) {}

  ImageType get_image_type() { return ImageType::RGB; }

  Resolution get_resolution() {
    std::lock_guard<std::mutex> lock(mutex);
    Resolution resolution;
    resolution.width = webcam.get(cv::CAP_PROP_FRAME_WIDTH);
    resolution.height = webcam.get(cv::CAP_PROP_FRAME_HEIGHT);
    return resolution;
  }

  void set_delay(Delay) {}

  cv::Mat get_frame() {
    cv::Mat frame;
    std::lock_guard<std::mutex> lock(mutex);
    webcam >> frame;
    last_timestamp = TimeStamp();
    return frame;
  }

  TimeStamp get_last_timestamp() {
    std::lock_guard<std::mutex> lock(mutex);
    return last_timestamp;
  }
};

}  // ::driver
}  // ::is

#endif  // __IS_DRIVER_WEBCAM_HPP__
