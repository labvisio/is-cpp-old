#ifndef __IS_GW_CAMERA_HPP__
#define __IS_GW_CAMERA_HPP__

#include <is/is.hpp>
#include <is/msgs/camera.hpp>
#include <is/msgs/common.hpp>
#include <is/msgs/cv.hpp>

namespace is {
namespace gw {

using namespace is::msg::common;
using namespace is::msg::camera;

template <typename ThreadSafeCameraDriver>
struct Camera {
  is::Connection is;

  Camera(std::string const& name, std::string const& uri, ThreadSafeCameraDriver & camera) : is(is::connect(uri)) {
    auto thread = is::advertise(uri, name, {
      {
        "set_sample_rate",
        [&](is::Request request) -> is::Reply {
          camera.set_sample_rate(is::msgpack<SamplingRate>(request));
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
        "set_image_type", 
        [&](is::Request request) -> is::Reply {
          camera.set_image_type(is::msgpack<ImageType>(request));
          return is::msgpack(Status::OK);
        }
      },
      {
        "set_delay", 
        [&](is::Request request) -> is::Reply {
          camera.set_delay(is::msgpack<Delay>(request));
          return is::msgpack(Status::OK);
        }
      },
      {
        "get_sample_rate",
        [&](is::Request) -> is::Reply {
          return is::msgpack(camera.get_sample_rate());
        }
      },
      {
        "get_resolution", 
        [&](is::Request) -> is::Reply {
          return is::msgpack(camera.get_resolution());
        }
      },
      {
        "get_image_type", 
        [&](is::Request) -> is::Reply {
          return is::msgpack(camera.get_image_type());
        }
      }
    });
    
    while (1) {
      cv::Mat frame = camera.get_frame();
      TimeStamp ts = camera.get_last_timestamp();
      CompressedImage image;
      image.format = ".png";
      cv::imencode(image.format, frame, image.data);
      is.publish(name + ".frame", is::msgpack(image));      
      is.publish(name + ".timestamp", is::msgpack(ts));
    }

    thread.join();
  }
};

}  // ::gw
}  // ::is

#endif // __IS_GW_CAMERA_HPP__