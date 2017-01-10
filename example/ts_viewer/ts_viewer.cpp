#include <boost/program_options.hpp>
#include <iostream>
#include <opencv2/highgui.hpp>
#include "drivers/webcam.hpp"
#include "gateways/camera.hpp"
#include "is.hpp"

using namespace std::chrono;
using namespace is::msg::camera;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::string uri;
  std::string entity;
  is::msg::camera::Resolution resolution;
  double fps;
  std::string imtype_str;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("entity,e", po::value<std::string>(&entity), "entity name");
  options("height,h", po::value<int>(&resolution.height)->default_value(480), "image height");
  options("width,w", po::value<int>(&resolution.width)->default_value(640), "image width");
  options("fps,f", po::value<double>(&fps)->default_value(30.0), "frames per second");
  options("type,t", po::value<std::string>(&imtype_str)->default_value("RGB"), "image type");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("entity")) {
    std::cout << description << std::endl;
    return 1;
  }

  auto is = is::connect(uri);
  auto client = is::make_client(is);
  client.request(entity + ".set_fps", is::msgpack(fps));
  client.request(entity + ".set_resolution", is::msgpack(resolution));
  
  if (imtype_str == "RGB") {
    client.request(entity + ".set_image_type", is::msgpack(ImageType::RGB));
  }
  else if (imtype_str == "GRAY") {
    client.request(entity + ".set_image_type", is::msgpack(ImageType::GRAY));
  }

  auto timestamps = is.subscribe({entity + ".timestamp"});

  is::msg::common::TimeStamp old_ts, ts;
  while (1) {
    auto message = is.consume(timestamps);
    old_ts = ts;
    ts = is::msgpack<is::msg::common::TimeStamp>(message);
    auto diff = nanoseconds(ts.time_point - old_ts.time_point);
    double dt = duration_cast<microseconds>(diff).count();
    std::cout << dt << std::endl;
  }
  return 0;
}