#include "drivers/pioneer.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include "gateways/robot.hpp"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::string uri;
  std::string entity;
  std::string robot_serial_port;
  std::string robot_hostname;
  int robot_port;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("entity,e", po::value<std::string>(&entity), "entity name");
  options("serialport,s", po::value<std::string>(&robot_serial_port), "robot serial port");
  options("hostname,h", po::value<std::string>(&robot_hostname), "robot hostname");
  options("port,p", po::value<int>(&robot_port), "robot port");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("entity")) {
    std::cout << description << std::endl;
    return 1;
  }
  is::driver::Pioneer robot;

  if (vm.count("serialport") && !vm.count("hostname")) {
    robot.connect(robot_serial_port);
  } else if (vm.count("hostname") && vm.count("port")) {
    robot.connect(robot_hostname, robot_port);
  } else {
    std::cout << description << std::endl;
    return 1;
  }

  is::gw::Robot<is::driver::Pioneer> gw(entity, uri, robot);
  return 0;
}