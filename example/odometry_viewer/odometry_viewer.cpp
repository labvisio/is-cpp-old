#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include "gateways/robot.hpp"
#include "is.hpp"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::string uri;
  std::string entity;
  std::string pose_str;
  double v, w;
  int64_t period;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("entity,e", po::value<std::string>(&entity), "entity name");
  options("linvel,v", po::value<double>(&v), "linear velocity in mm/s");
  options("rotvel,w", po::value<double>(&w), "rotational velocity in deg/s");
  options("initial_pose,p", po::value<std::string>(&pose_str),
          "initial pose (x[mm],y[mm],th[deg]) e.g.: 0.0,100.0,30.0");
  options("period,t", po::value<int64_t>(&period), "sampling period in ms");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("entity")) {
    std::cout << description << std::endl;
    return 1;
  }

  std::cout << pose_str << std::endl;

  auto is = is::connect(uri);
  auto client = is::make_client(is);

  if (vm.count("initial_pose")) {
    std::vector<std::string> fields;
    boost::split(fields, pose_str, boost::is_any_of(","), boost::token_compress_on);
    if (fields.size() == 3) {
      is::msg::robot::Odometry pose{stod(fields[0]), stod(fields[1]), stod(fields[2])};
      client.request(entity + ".set_pose", is::msgpack(pose));
    }
  }

  if (vm.count("linvel") && vm.count("rotvel")) {
    is::msg::robot::Velocities vels{v, w};
    client.request(entity + ".set_velocities", is::msgpack(vels));
  }

  if (vm.count("period")) {
    is::msg::robot::SamplingRate sample_rate { period };
    client.request(entity + ".set_sample_rate", is::msgpack(sample_rate));
  }

  auto odometries = is.subscribe({entity + ".odometry"});

  while (1) {
    auto message = is.consume(odometries);
    auto odo = is::msgpack<is::msg::robot::Odometry>(message);
    std::cout << odo.x << '\t' << odo.y << '\t' << odo.th << '\n';
  }
  return 0;
}