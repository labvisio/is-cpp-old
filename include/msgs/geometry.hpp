#ifndef __IS_MSG_GEOMETRY_HPP__
#define __IS_MSG_GEOMETRY_HPP__

#include "../packer.hpp"

namespace is {
namespace msg {
namespace geometry {

struct Point {
  double x;
  double y;
  boost::optional<double> z = boost::none;
  IS_DEFINE_MSG(x, y, z);
};

struct PointsWithReference {
  std::string reference;
  std::vector<Point> points;
  IS_DEFINE_MSG(reference, points);
};

}  // ::geometry
}  // ::msg
}  // ::is

#endif  // __IS_MSG_GEOMETRY_HPP__
