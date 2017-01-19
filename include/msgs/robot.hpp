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
  Odometry current, desired;
  std::string odometry_source;
  bool external_source;
  double kpx, kpy, lx, ly, a;

  FinalPosition() {}
  
  FinalPosition(Odometry current, Odometry desired) {
    this->current = current;
    this->desired = desired;
    kpx = 0.2;
    kpy = 0.2;
    lx = 80;
    ly = 80;
    a = 200.0; // [mm]
    external_source = false;
  }

  FinalPosition(std::string odometry_source, Odometry desired) {
    this->odometry_source = odometry_source;
    this->desired = desired;
    kpx = 0.2;
    kpy = 0.2;
    lx = 80;
    ly = 80;
    a = 200.0; // [mm]
    external_source = true;
  }

  MSGPACK_DEFINE_ARRAY(current, desired, odometry_source, external_source, kpx, kpy, lx, ly, a);
};

}  // ::robot
}  // ::msg
}  // ::is

#endif  // __IS_MSG_ROBOT_HPP__