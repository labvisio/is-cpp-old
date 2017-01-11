#ifndef __IS_DRIVER_PTGREY_HPP__
#define __IS_DRIVER_PTGREY_HPP__

#include <flycapture/FlyCapture2.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <mutex>
#include <numeric>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <string>
#include "../logger.hpp"
#include "../msgs/camera.hpp"
#include "../msgs/common.hpp"

namespace is {
namespace driver {

using namespace std::chrono_literals;
using namespace is::msg::camera;
using namespace is::msg::common;
using namespace FlyCapture2;

struct ErrorLogger {
  ErrorLogger() {}

  ErrorLogger(Error error) {
    if (error != PGRERROR_OK) {
      is::logger()->warn("[Ptgrey][{}][{}] {}", error.GetFilename(), error.GetLine(), error.GetDescription());
    }
  }
};

struct PtGrey {
  std::mutex mutex;

  GigECamera camera;
  PGRGuid* handle;

  ImageType image_type;
  PixelFormat pixel_format;
  int cv_type;
  TimeStamp last_timestamp;

  ErrorLogger error;

  PtGrey(std::string const& ip) : handle(new PGRGuid()) {
    BusManager bus;
    bus.GetCameraFromIPAddress(make_ip_address(ip), handle);
    camera.Connect(handle);
    set_packet_delay(6000);
    set_packet_size(1400);
    set_image_type(ImageType::RGB);
    set_sample_rate({1.0});
    camera.StartCapture();
  }

  ~PtGrey() {
    camera.Disconnect();
    if (handle != nullptr) {
      delete handle;
    }
  }

  void set_sample_rate(SamplingRate sample_rate) {
    std::lock_guard<std::mutex> lock(mutex);

    Property property(FRAME_RATE);
    error = camera.GetProperty(&property);

    PropertyInfo info(FRAME_RATE);
    error = camera.GetPropertyInfo(&info);

    float fps;
    if (sample_rate.rate == 0.0 && sample_rate.period > 0) {
      fps = static_cast<float>(1000 / sample_rate.period);
    } else if (sample_rate.rate > 0.0 && sample_rate.period == 0) {
      fps = static_cast<float>(sample_rate.rate);
    }

    property.absValue = std::max(info.absMin, std::min(fps, info.absMax));
    property.autoManualMode = false;

    error = camera.SetProperty(&property);
  }

  SamplingRate get_sample_rate() {
    std::lock_guard<std::mutex> lock(mutex);
    Property property(FRAME_RATE);
    error = camera.GetProperty(&property);
    return {static_cast<double>(property.absValue)};
  }

  double get_max_fps() {
    std::lock_guard<std::mutex> lock(mutex);
    PropertyInfo info(FRAME_RATE);
    error = camera.GetPropertyInfo(&info);
    return static_cast<double>(info.absMax);
  }

  void set_resolution(Resolution resolution) {
    std::lock_guard<std::mutex> lock(mutex);
    if (resolution.width == 1288 && resolution.height == 728) {
      camera.StopCapture();
      error = camera.SetGigEImagingMode(MODE_0);
      camera.StartCapture();
    } else if (resolution.width == 644 && resolution.height == 364) {
      camera.StopCapture();
      error = camera.SetGigEImagingMode(MODE_1);
      camera.StartCapture();
    }
  }

  Resolution get_resolution() {
    std::lock_guard<std::mutex> lock(mutex);
    Mode mode;
    camera.GetGigEImagingMode(&mode);
    switch (mode) {
      case MODE_0:
        return {1288, 728};
      case MODE_1:
        return {644, 364};
      default:
        return {0, 0};
    }
  }

  void set_image_type(ImageType image_type) {
    std::lock_guard<std::mutex> lock(mutex);
    this->image_type = image_type;
    switch (image_type) {
      case ImageType::GRAY:
        pixel_format = FlyCapture2::PIXEL_FORMAT_MONO8;
        cv_type = CV_8UC1;
        break;
      default:
        pixel_format = FlyCapture2::PIXEL_FORMAT_BGR;
        cv_type = CV_8UC3;
        break;
    }
  }

  ImageType get_image_type() { return image_type; }

  void set_delay(Delay delay) {
    std::lock_guard<std::mutex> lock(mutex);

    Property property(TRIGGER_DELAY);
    error = camera.GetProperty(&property);

    PropertyInfo info(TRIGGER_DELAY);
    error = camera.GetPropertyInfo(&info);

    property.absValue = std::max(info.absMin, std::min(static_cast<float>(delay.milliseconds) / 1000.0f, info.absMax));
    property.onOff = true;

    error = camera.SetProperty(&property);
  }

  cv::Mat get_frame() {
    std::lock_guard<std::mutex> lock(mutex);

    Image buffer, image;
    error = camera.RetrieveBuffer(&buffer);
    last_timestamp = TimeStamp();
    buffer.Convert(pixel_format, &image);
    cv::Mat frame(image.GetRows(), image.GetCols(), cv_type, image.GetData(), image.GetDataSize() / image.GetRows());
    frame = frame.clone();
    buffer.ReleaseBuffer();
    image.ReleaseBuffer();
    return frame;
  }

  TimeStamp get_last_timestamp() {
    std::lock_guard<std::mutex> lock(mutex);
    return last_timestamp;
  }

 private:
  void set_property(unsigned int value, GigEPropertyType type) {
    GigEProperty property;
    property.propType = type;
    error = camera.GetGigEProperty(&property);
    property.value = std::max(property.min, std::min(value, property.max));
    error = camera.SetGigEProperty(&property);
  }

  void set_packet_delay(unsigned int delay) { set_property(delay, FlyCapture2::PACKET_DELAY); }

  void set_packet_size(unsigned int size) { set_property(size, FlyCapture2::PACKET_SIZE); }

  IPAddress make_ip_address(std::string const& ip) {
    std::vector<std::string> fields;
    boost::split(fields, ip, boost::is_any_of("."), boost::token_compress_on);
    auto reducer = [i = 8 * (fields.size() - 1)](auto total, auto current) mutable {
      total += (std::stoi(current) << i);
      i -= 8;
      return total;
    };
    IPAddress address = std::accumulate(std::begin(fields), std::end(fields), 0, reducer);
    return address;
  }
};

}  // ::driver
}  // ::is

#endif  // __IS_DRIVER_PTGREY_HPP__