#include "is.hpp"
#include "msgs/common.hpp"
#include <boost/program_options.hpp>
#include <iostream>

using namespace is::msg::common;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::string uri;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << description << std::endl;
    return 1;
  }

  auto is = is::connect(uri);
  auto client = is::make_client(is);
  
  EntityList entities({"ptgrey.0", "ptgrey.1", "ptgrey.2", "ptgrey.3"});
  client.request("is.sync", is::msgpack(entities));

  return 0;
}