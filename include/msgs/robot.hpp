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

}  // ::robot
}  // ::msg
}  // ::is

#endif  // __IS_MSG_ROBOT_HPP__