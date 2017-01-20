#ifndef __IS_MSG_ROBOT_HPP__
#define __IS_MSG_ROBOT_HPP__

#include "../packer.hpp"

namespace is {
namespace msg {
namespace robot {

struct Odometry {
  double x;   //  [mm]
  double y;   //  [mm]
  double th;  //  [deg]

  MSGPACK_DEFINE_ARRAY(x, y, th);
};

struct Velocities {
  double v;  //  [mm/s]
  double w;  //  [deg/s]

  MSGPACK_DEFINE_ARRAY(v, w);
};

struct FinalPosition {
  // one of {
  Odometry current;
  std::string odometry_source;
  // }
  Odometry desired;
  double gain_x;
  double gain_y;
  double max_vel_x;
  double max_vel_y;
  double center_offset;
  
  MSGPACK_DEFINE_ARRAY(current, odometry_source, desired, gain_x, gain_y, max_vel_x, max_vel_y, center_offset);
};

}  // ::robot
}  // ::msg
}  // ::is

#endif  // __IS_MSG_ROBOT_HPP__