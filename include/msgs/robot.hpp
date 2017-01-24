#ifndef __IS_MSG_ROBOT_HPP__
#define __IS_MSG_ROBOT_HPP__

#include "../packer.hpp"

namespace is {
namespace msg {
namespace robot {

struct Pose {
  is::msg::geometry::Point position;  // [mm]
  double heading;                     // [deg]
  IS_DEFINE_MSG(position, heading);
};

struct Speed {
  double linear;   // [mm/s]
  double angular;  // [deg/s]
  IS_DEFINE_MSG(linear, angular);
};

struct ControlActionRequest {
  Pose current_pose;
  Pose desired_pose;

  double gain_x;
  double gain_y;
  double max_vel_x;
  double max_vel_y;
  double center_offset;

  IS_DEFINE_MSG(current_pose, desired_pose, gain_x, gain_y, max_vel_x, max_vel_y, center_offset);
};

}  // ::robot
}  // ::msg
}  // ::is

#endif  // __IS_MSG_ROBOT_HPP__