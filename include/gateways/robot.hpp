#ifndef __IS_GW_ROBOT_HPP__
#define __IS_GW_ROBOT_HPP__

#include "../is.hpp"
#include "../msgs/common.hpp"
#include "../msgs/robot.hpp"

namespace is {
namespace gw {

using namespace is::msg::common;
using namespace is::msg::robot;

template <typename ThreadSafeRobotDriver>
struct Robot {
  is::Connection is;

  Robot(std::string const& name, std::string const& uri, ThreadSafeRobotDriver & robot) : is(is::connect(uri)) {
    auto thread = is::advertise(uri, name, {
      {
        "set_velocities",
        [&](is::Request request) -> is::Reply {
          robot.set_velocities(is::msgpack<Velocities>(request));
          return is::msgpack(Status::OK);
        }
      },
      {
        "get_velocities",
        [&](is::Request request) -> is::Reply {
          return is::msgpack(robot.get_velocities());
        }
      },
      {
        "set_pose",
        [&](is::Request request) -> is::Reply {
          robot.set_pose(is::msgpack<Odometry>(request));
          return is::msgpack(Status::OK);
        }
      },
      {
        "set_sample_rate",
        [&](is::Request request) -> is::Reply {
          robot.set_sample_rate(is::msgpack<SamplingRate>(request));
          return is::msgpack(Status::OK);
        }
      },
      {
        "get_sample_rate",
        [&](is::Request request) -> is::Reply {
          return is::msgpack(robot.get_sample_rate());
        }
      }
    });
    
    while (1) {
      is.publish(name + ".odometry", is::msgpack(robot.get_odometry()));
      is.publish(name + ".timestamp", is::msgpack(robot.get_last_timestamp()));
    }

    thread.join();
  }
};

}  // ::gw
}  // ::is

#endif  // __IS_GW_ROBOT_HPP__