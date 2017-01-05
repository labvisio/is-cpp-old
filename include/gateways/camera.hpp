#ifndef __IS_GW_CAMERA_HPP__
#define __IS_GW_CAMERA_HPP__

#include "../is.hpp"
#include "../msgs/camera.hpp"
#include "../msgs/common.hpp"
#include "../msgs/cv.hpp"

namespace is {
namespace gw {

using namespace is::msg::common;
using namespace is::msg::camera;

template <typename ThreadSafeCameraDriver>
struct Camera {
  ThreadSafeCameraDriver camera;
  is::Connection is;

  Camera(std::string const& name, std::string const& uri) : is(is::connect(uri)) {
    auto thread = is::advertise(uri, name, {
      {
        "set_fps",
        [&](is::Request request) -> is::Reply {
          auto fps = is::msgpack<double>(request);
          is::logger()->info("Setting fps to {}", fps);
          camera.set_fps(fps);
          return is::msgpack(Status::OK);
        }
      },
      {
        "set_resolution", 
        [&](is::Request request) -> is::Reply {
          camera.set_resolution(is::msgpack<Resolution>(request));
          return is::msgpack(Status::OK);
        }
      },
      {
        "get_fps",
        [&](is::Request) -> is::Reply {
          return is::msgpack(camera.get_fps());
        }
      },
      {
        "get_resolution", 
        [&](is::Request) -> is::Reply {
          return is::msgpack(camera.get_resolution());
        }
      }
    });
    
    while (1) {
      cv::Mat frame = camera.get_frame();
      CompressedImage image;
      image.format = ".png";
      cv::imencode(image.format, frame, image.data);
      is.publish(name + ".frame", is::msgpack(image));
    }

    thread.join();
  }
};

}  // ::gw
}  // ::is

#endif // __IS_GW_CAMERA_HPP__
