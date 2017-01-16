#ifndef __IS_MSG_COMMON_HPP__
#define __IS_MSG_COMMON_HPP__

#include <chrono>
#include <string>
#include <vector>
#include "../packer.hpp"

namespace is {
namespace msg {
namespace common {

enum class Status { OK, FAILED };

struct Delay {
  int milliseconds;

  MSGPACK_DEFINE_ARRAY(milliseconds);
};

struct TimeStamp {
  int64_t time_point;

  TimeStamp() { time_point = std::chrono::system_clock::now().time_since_epoch().count(); }

  MSGPACK_DEFINE_ARRAY(time_point);
};

struct Point2d {
  double x, y;

  MSGPACK_DEFINE_ARRAY(x, y);
};

struct Pattern {
  std::vector<Point2d> points;
  bool found;

  MSGPACK_DEFINE_ARRAY(points, found);
};

}  // ::common
}  // ::msg
}  // ::is

// Enum packing must be done on global namespace
MSGPACK_ADD_ENUM(is::msg::common::Status);

#endif  // __IS_MSG_COMMON_HPP__
