#ifndef __IS_DRIVER_PTGREY_HPP__
#define __IS_DRIVER_PTGREY_HPP__

#include <flycapture/FlyCapture2.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>
#include "../msgs/camera.hpp"

namespace is {
namespace driver {

using namespace std::chrono_literals;
using namespace is::msg::camera;
using namespace FlyCapture2;

struct PtGrey {
  GigECamera camera;
  PGRGuid* handle;
  std::mutex mutex;
  ImageType cur_imtype;

  PtGrey(std::string const& ip_address) {
    std::lock_guard<std::mutex> lock(mutex);
    handle = new PGRGuid();
    Error error;
    get_handle(ip_address);
    error = camera.Connect(handle);
    if (error != PGRERROR_OK) {
      error.PrintErrorTrace();
    }
    set_packet_delay(6000);
    set_packet_size(1400);
    cur_imtype = ImageType::RGB;
    camera.StartCapture();
  }

  ~PtGrey() {
    camera.Disconnect();
    if (handle != nullptr)
      delete handle;
  }

  void set_fps(double fps) {
    mutex.lock();
    Error error;
    Property frmRate;
    frmRate.type = FRAME_RATE;
    error = camera.GetProperty(&frmRate);
    if (error != PGRERROR_OK) {
      error.PrintErrorTrace();
    }
    // validate FPS
    mutex.unlock();
    double max_fps = get_max_fps();
    mutex.lock();
    fps = fps > max_fps ? max_fps : fps;

    frmRate.autoManualMode = false;
    frmRate.absValue = static_cast<float>(fps);
    error = camera.SetProperty(&frmRate);
    if (error != PGRERROR_OK) {
      error.PrintErrorTrace();
    }
    // set_trigger_delay(0.0);
    // query_max_trigger_delay();
    mutex.unlock();
  }

  double get_fps() {
    std::lock_guard<std::mutex> lock(mutex);
    Error error;
    Property frmRate;
    frmRate.type = FRAME_RATE;
    error = camera.GetProperty(&frmRate);
    if (error != PGRERROR_OK) {
      error.PrintErrorTrace();
    }
    return static_cast<double>(frmRate.absValue);
  }

  double get_max_fps() {
    std::lock_guard<std::mutex> lock(mutex);
    Error error;
    PropertyInfo frmRateInfo;
    frmRateInfo.type = FRAME_RATE;
    error = camera.GetPropertyInfo(&frmRateInfo);
    if (error != PGRERROR_OK) {
      error.PrintErrorTrace();
    }
    return static_cast<double>(frmRateInfo.absMax);
  }

  void set_resolution(Resolution resolution) {
    std::lock_guard<std::mutex> lock(mutex);
    Error error;
    if (resolution.width == 1288 && resolution.height == 728) {
      camera.StopCapture();
      error = camera.SetGigEImagingMode(MODE_0);
      if (error != PGRERROR_OK) {
        error.PrintErrorTrace();
      }
      camera.StartCapture();
    } else if (resolution.width == 644 && resolution.height == 364) {
      camera.StopCapture();
      error = camera.SetGigEImagingMode(MODE_1);
      if (error != PGRERROR_OK) {
        error.PrintErrorTrace();
      }
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
    cur_imtype = image_type;
  }

  ImageType get_image_type() { return cur_imtype; }

  cv::Mat get_frame() {
    std::lock_guard<std::mutex> lock(mutex);
    Image image, converted_image;
    Error error;
    error = camera.RetrieveBuffer(&image);
    if (error != PGRERROR_OK) {
      error.PrintErrorTrace();
    }
    PixelFormat format;
    int cvtype;
    switch (cur_imtype) {
      case ImageType::GRAY:
        format = FlyCapture2::PIXEL_FORMAT_MONO8;
        cvtype = CV_8UC1;
        break;
      default:
        format = FlyCapture2::PIXEL_FORMAT_BGR;
        cvtype = CV_8UC3;
        break;
    }

    image.Convert(format, &converted_image);
    cv::Mat frame(converted_image.GetRows(), converted_image.GetCols(), cvtype, converted_image.GetData(),
                  converted_image.GetDataSize() / converted_image.GetRows());
    cv::Mat ret_frame;
    ret_frame = frame.clone();
    image.ReleaseBuffer();
    converted_image.ReleaseBuffer();
    return ret_frame;
  }

  void set_packet_delay(unsigned int delay) {
    Error error;
    GigEProperty p;
    p.propType = FlyCapture2::PACKET_DELAY;
    error = this->camera.GetGigEProperty(&p);
    if (error != PGRERROR_OK) {
      error.PrintErrorTrace();
      return;
    }
    delay = delay > p.max ? p.max : delay;
    delay = delay < p.min ? p.min : delay;
    p.value = delay;
    error = this->camera.SetGigEProperty(&p);
    if (error != PGRERROR_OK) {
      error.PrintErrorTrace();
    }
  }

  void set_packet_size(unsigned int size) {
    Error error;
    GigEProperty p;
    p.propType = FlyCapture2::PACKET_SIZE;
    error = this->camera.GetGigEProperty(&p);
    if (error != PGRERROR_OK) {
      error.PrintErrorTrace();
      return;
    }
    size = size > p.max ? p.max : size;
    size = size < p.min ? p.min : size;
    p.value = size;
    error = this->camera.SetGigEProperty(&p);
    if (error != PGRERROR_OK) {
      error.PrintErrorTrace();
    }
  }

  /*-------------------------------------------------------------------*/

  /*-------------------------------------------------------------------*/
  void get_handle(const std::string& ip_address) {
    BusManager bus;
    Error error;
    int ip_int = ip_to_int(ip_address.c_str());
    if (!ip_int) {
      throw std::runtime_error("Invalid IP address.");
    }
    IPAddress ip(ip_int);
    error = bus.GetCameraFromIPAddress(ip, handle);
    if (error != PGRERROR_OK) {
      error.PrintErrorTrace();
      throw std::runtime_error("Camera not found.");
    }
  }

  std::vector<std::string> split(const std::string& s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim)) {
      elems.push_back(item);
    }
    return elems;
  }

  int ip_to_int(const char* ip) {
    std::string data(ip);
    auto split_data = split(data, '.');
    int IP = 0;
    if (split_data.size() == 4) {
      int sl_num = 24;
      for (auto& d : split_data) {
        int octets = stoi(d);
        if ((octets >> 8) && 0xffffff00) {
          return 0;  // error
        }
        IP += (octets << sl_num);
        sl_num -= 8;
      }
      return IP;
    }
    return 0;
  }
};

}  // ::driver
}  // ::is

#endif  // __IS_DRIVER_PTGREY_HPP__