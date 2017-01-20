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

  double z = 650.0;
  mat I;
  I.load("/home/felippe/Desktop/" + camera + "/intrinsic.dat");
  mat E;
  E.load("/home/felippe/Desktop/" + camera + "/extrinsic.dat");

  I.print("I");
  E.print("E");

  mat A = I * eye(3, 4) * E;
  mat X;
  auto image_to_world = [&](Point2d p) {
    vec image = {p.x, p.y, 1.0};
    mat world = inv(join_horiz(A.cols(0, 1), -image)) * (-A.col(3) - z * A.col(2));
    return world;
  };

  auto get_pose = [&](Pattern pattern) {
    Odometry odo;
    if (pattern.found) {
      Point2d image_center = {(pattern.points[4].x + pattern.points[7].x) / 2.0,
                              (pattern.points[4].y + pattern.points[7].y) / 2.0};
      mat center = image_to_world(image_center);
      mat p0 = image_to_world(pattern.points[0]);
      mat p2 = image_to_world(pattern.points[2]);
      double th = rad2deg(std::atan2(p2.at(1) - p0.at(1), p2.at(0) - p0.at(0)));
      odo = {center.at(0), center.at(1), th};
    } else {
      odo = {0.0, 0.0, 0.0};
    }
    return odo;
  };

  is::Connection is(is::connect(uri));

  auto thread = is::advertise(uri, name, {{"find",
                                           [&](is::Request request) -> is::Reply {
                                             auto image = is::msgpack<is::msg::camera::CompressedImage>(request);
                                             return is::msgpack<Pattern>(find_pattern(image));
                                           }},
                                          {"get_pose", [&](is::Request request) -> is::Reply {
                                             auto pattern = is::msgpack<is::msg::common::Pattern>(request);
                                             return is::msgpack(get_pose(pattern));
                                           }},
                                          {"image_to_world", [&](is::Request request) -> is::Reply {
                                             auto image = is::msgpack<is::msg::common::Point2d>(request);
                                             auto world = image_to_world(image);
                                             auto odometry = Odometry{world.at(0), world.at(1), 0.0};
                                             return is::msgpack(odometry);
                                           }}});

  thread.join();
  return 0;
}