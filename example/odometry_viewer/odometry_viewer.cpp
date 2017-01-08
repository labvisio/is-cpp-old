#include <boost/program_options.hpp>
#include <iostream>
#include "gateways/robot.hpp"
#include "is.hpp"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::string uri;
  std::string entity;
  double v, w;
  is::msg::robot::Velocities vels;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("entity,e", po::value<std::string>(&entity), "entity name");
  options("linvel,v", po::value<double>(&v), "linear velocity in mm/s");
  options("rotvel,w", po::value<double>(&w), "rotational velocity in deg/s");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("entity")) {
    std::cout << description << std::endl;
    return 1;
  }

  auto is = is::connect(uri);
  auto client = is::make_client(is);
  vels = {v, w};
  client.request(entity + ".set_velocities", is::msgpack(vels));

  auto odometries = is.subscribe({entity + ".odometry"});

  while (1) {
    auto message = is.consume(odometries);
    auto odo = is::msgpack<is::msg::robot::Odometry>(message);
    std::cout << odo.x << '\t' << odo.y << '\t' << odo.th << '\n';
  }
  return 0;
}