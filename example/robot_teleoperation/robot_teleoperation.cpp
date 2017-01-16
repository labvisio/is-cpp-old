#include <boost/program_options.hpp>
#include <iostream>
#include "drivers/pioneer.hpp"
#include "gateways/robot.hpp"
#include "is.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <chrono>
#include </home/felippe/joystick/joystick.hh>

namespace po = boost::program_options;
using namespace is::msg::robot;

int main(int argc, char* argv[]) {
  std::string uri;
  std::string entity;
  std::string joystick_name;
  double max_v;
  double max_w;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("entity,e", po::value<std::string>(&entity), "entity name");
  options("maxlinvel,v", po::value<double>(&max_v)->default_value(400), "max linear velocity");
  options("maxrotvel,w", po::value<double>(&max_w)->default_value(20), "max rotational velocity");
  options("joystcik,j", po::value<std::string>(&joystick_name)->default_value("/dev/input/js0"), "joystcik name");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("entity")) {
    std::cout << description << std::endl;
    return 1;
  }

  auto is = is::connect(uri);
  auto client = is::make_client(is);

  Odometry ini_odo{0.0, 0.0, 0.0};
  Velocities ini_vels{0.0, 0.0};

  // client.request(entity + ".set_pose", is::msgpack(ini_odo));
  // client.request(entity + ".set_velocities", is::msgpack(ini_vels));

  // Create an instance of Joystick
  Joystick joystick(joystick_name);
  // Ensure that it was found and that we can use it
  if (!joystick.isFound()) {
    std::cout << "Joystick not found." << std::endl;
    exit(1);
  }

  while (1) {
    Velocities vels;
    double v = 0.0;
    double w = 0.0;
    // Attempt to sample an event from the joystick
    JoystickEvent event;
    if (joystick.sample(&event)) {
      if (event.isAxis()) {
        std::cout << event.number << '\t' << event.value << std::endl;
        switch (event.number) {
          case 0:
            w = max_w * (event.value / 32767.0);
            break;
          case 1:
            v = max_v * (event.value / 32767.0);
            break;
        }
        vels = {v, w};
        client.request(entity + ".set_velocities", is::msgpack(vels));
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return 0;
}