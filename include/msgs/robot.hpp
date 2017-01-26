#ifndef __IS_MSG_ROBOT_HPP__
#define __IS_MSG_ROBOT_HPP__

#include "../packer.hpp"
#include "geometry.hpp"

namespace is {
namespace msg {
namespace robot {

struct Pose {
  is::msg::geometry::Point position;  // [mm]
  double heading;                     // [rad]
  IS_DEFINE_MSG(position, heading);
};

struct Speed {
  double linear;   // [mm/s]
  double angular;  // [rad/s]
  IS_DEFINE_MSG(linear, angular);
};

}  // ::robot
}  // ::msg
}  // ::is

#endif  // __IS_MSG_ROBOT_HPP__