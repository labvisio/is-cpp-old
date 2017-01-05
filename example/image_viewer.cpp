#include <boost/program_options.hpp>
#include <iostream>
#include <opencv2/highgui.hpp>
#include "drivers/webcam.hpp"
#include "gateways/camera.hpp"
#include "is.hpp"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::string uri;
  std::string entity;
  is::msg::camera::Resolution resolution;
  double fps;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("entity,e", po::value<std::string>(&entity), "entity name");
  options("height,h", po::value<int>(&resolution.height)->default_value(480), "image height");
  options("width,w", po::value<int>(&resolution.width)->default_value(640), "image width");
  options("fps,f", po::value<double>(&fps)->default_value(30.0), "frames per second");

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

  auto frames = is.subscribe({entity + ".frame"});

  while (1) {
    auto message = is.consume(frames);
    auto image = is::msgpack<is::msg::camera::CompressedImage>(message);
    cv::Mat frame = cv::imdecode(image.data, CV_LOAD_IMAGE_COLOR);
    cv::imshow("Webcam stream", frame);
    cv::waitKey(1);
  }
  return 0;
}