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

struct SamplingRate {
  double rate;     // [Hz]
  int64_t period;  // [ms]

  SamplingRate() {}

  SamplingRate(double r) {
    rate = r;
    period = 0;
  }

  SamplingRate(int64_t p) {
    period = p;
    rate = 0.0;
  }

  MSGPACK_DEFINE_ARRAY(rate, period);
};

}  // ::robot
}  // ::msg
}  // ::is

#endif  // __IS_MSG_ROBOT_HPP__