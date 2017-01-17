#include <boost/program_options.hpp>
#include <iostream>
#include <is/drivers/webcam.hpp>
#include <is/gateways/camera.hpp>

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::string uri;
  std::string entity;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("entity,e", po::value<std::string>(&entity), "entity name");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("entity")) {
    std::cout << description << std::endl;
    return 1;
  }

  is::driver::Webcam camera;
  is::gw::Camera<is::driver::Webcam> gw(entity, uri, camera);
  return 0;
}