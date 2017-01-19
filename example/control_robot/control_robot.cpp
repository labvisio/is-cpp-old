#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
#include <cmath>
#include <iostream>
#include <is/is.hpp>
#include <is/msgs/common.hpp>
#include "../../include/msgs/robot.hpp"

namespace po = boost::program_options;
using namespace is::msg::robot;
using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
  std::string uri;
  std::string entity;
  std::string desired_pose_str;
  int64_t period;
  Odometry desired{0.0, 0.0, 0.0};

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("entity,e", po::value<std::string>(&entity), "entity name");
  options("desired_pose,p", po::value<std::string>(&desired_pose_str), "derided pose (x[mm],y[mm]) e.g.: 0.0,100.0");
  options("period,t", po::value<int64_t>(&period)->default_value(100), "sampling period in ms");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("entity")) {
    std::cout << description << std::endl;
    return 1;
  }

  auto is = is::connect(uri);
  auto client = is::make_client(is);

  if (vm.count("desired_pose")) {
    std::vector<std::string> fields;
    boost::split(fields, desired_pose_str, boost::is_any_of(","), boost::token_compress_on);
    if (fields.size() == 2) {
      desired = {stod(fields[0]), stod(fields[1]), 0.0};
    }
  }

  if (vm.count("period")) {
    is::msg::common::SamplingRate sample_rate{period};
    client.request(entity + ".set_sample_rate", is::msgpack(sample_rate));
  }

  while (client.receive(1s) != nullptr);

  auto odometries = is.subscribe({entity + ".odometry"});

  FinalPosition req_controller;
  req_controller.desired = desired;
  req_controller.gain_x = 0.2;
  req_controller.gain_y = 0.2;
  req_controller.max_vel_x = 80;
  req_controller.max_vel_y = 80;
  req_controller.center_offset = 200.0;

  while (1) {
    auto message = is.consume(odometries);
    Odometry current = is::msgpack<Odometry>(message);
    req_controller.current = current;
    auto req_id = client.request("controller.final_position", is::msgpack<FinalPosition>(req_controller));
    auto reply = client.receive(1s);

    if (reply == nullptr) {
      is::logger()->error("Controller request: timeout!");
    } else if (reply->Message()->CorrelationId() != req_id) {
      is::logger()->error("Controller request: Invalid reply");
    } else {
      req_id = client.request(entity + ".set_velocities", reply->Message());
    }

    Velocities vels = is::msgpack<Velocities>(reply);
    std::cout << "\r                                    \r"
              << "(x,y) (" << current.x << ';' << current.y << ") (v;w)(" << vels.v << ';' << vels.w << ')' << std::flush;

    reply = client.receive(1s);
    if (reply == nullptr) {
      is::logger()->error("Velocities request: timeout!");
    } else if (reply->Message()->CorrelationId() != req_id) {
      is::logger()->error("Velocities request: Invalid reply");
    }

    double p = std::sqrt(std::pow(desired.x - current.x, 2.0) + std::pow(desired.y - current.y, 2.0));
    if (p < 5.0) {
      client.request(entity + ".set_velocities", is::msgpack<Velocities>({0.0, 0.0}));
      return 0;
    }
  }
  return 0;
}