#include "find_pattern.hpp"
#include <armadillo>
#include <boost/program_options.hpp>
#include <cmath>
#include <is/is.hpp>
#include <is/msgs/common.hpp>
#include <is/msgs/robot.hpp>

namespace po = boost::program_options;
using namespace arma;
using namespace is::msg::camera;
using namespace is::msg::robot;
using namespace is::msg::common;

constexpr double pi() {
  return atan(1) * 4;
}
auto deg2rad = [](double deg) { return deg * (pi() / 180.0); };
auto rad2deg = [](double rad) { return rad * (180.0 / pi()); };

int main(int argc, char* argv[]) {
  std::string uri;
  std::string name{"pattern"};
  std::string camera;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("camera,c", po::value<std::string>(&camera), "camera parameters");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << description << std::endl;
    return 1;
  }

  auto get_pose = [&](std::vector<Point3d> points) {
    Odometry odo;
    if (!points.empty()) {
      double th = rad2deg(std::atan2(points[2].y - points[0].y, points[2].x - points[0].x));                        
      odo = {(points[4].x + points[7].x) / 2.0, (points[4].y + points[7].y) / 2.0, th};
    } else {
      odo = {0.0, 0.0, 0.0};
    }
    return odo;
  };

  is::Connection is(is::connect(uri));

  auto thread = is::advertise(uri, name, {{"find",
                                           [&](is::Request request) -> is::Reply {
                                             auto image = is::msgpack<CompressedImage>(request);
                                             return is::msgpack<Pattern>(find_pattern(image));
                                           }},
                                          {"get_pose", [&](is::Request request) -> is::Reply {
                                             auto points = is::msgpack<std::vector<Point3d>>(request);
                                             return is::msgpack(get_pose(points));
                                           }}});

  thread.join();
  return 0;
}