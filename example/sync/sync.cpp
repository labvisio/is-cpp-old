#include "sync_service_utils.hpp"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::string uri;
  std::string name {"is"};

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

  is::Connection is(is::connect(uri));
  
   auto thread = is::advertise(uri, name, {
      {
        "sync",
        [&](is::Request request) -> is::Reply {
          return is::msgpack(is::sync_entities(uri, is::msgpack<is::msg::common::EntityList>(request)));
        }
      }
   });

   thread.join();
}