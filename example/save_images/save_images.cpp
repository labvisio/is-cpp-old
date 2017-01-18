#include <boost/program_options.hpp>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <opencv2/highgui.hpp>
#include "drivers/webcam.hpp"
#include "gateways/camera.hpp"
#include "is.hpp"

using namespace is::msg::camera;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::string uri;
  std::string entity;
  is::msg::camera::Resolution resolution;
  is::msg::common::SamplingRate sample_rate;
  double fps;
  std::string imtype_str;
  
  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("entity,e", po::value<std::string>(&entity), "entity name");
  options("height,h", po::value<int>(&resolution.height)->default_value(480), "image height");
  options("width,w", po::value<int>(&resolution.width)->default_value(640), "image width");
  options("fps,f", po::value<double>(&fps)->default_value(10.0), "frames per second");
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
  
  sample_rate = {fps};
  client.request(entity + ".set_sample_rate", is::msgpack(sample_rate));
  client.request(entity + ".set_resolution", is::msgpack(resolution));
  
  if (imtype_str == "RGB") {
    client.request(entity + ".set_image_type", is::msgpack(ImageType::RGB));
  }
  else if (imtype_str == "GRAY") {
    client.request(entity + ".set_image_type", is::msgpack(ImageType::GRAY));
  }

  auto frames = is.subscribe({entity + ".frame"});
  int im_count = 0;

  char key = 0;
  do {
    auto message = is.consume(frames);
    auto image = is::msgpack<is::msg::camera::CompressedImage>(message);
    cv::Mat frame = cv::imdecode(image.data, CV_LOAD_IMAGE_COLOR);
    cv::imshow("Webcam stream", frame);
    key = cv::waitKey(1);
    if (key == 32) { // [Space]
      std::ostringstream filename;
		  filename << entity << '_' << std::setw(2) << std::setfill('0') << im_count++ << ".jpg";
      std::cout << filename.str() << " saved." << std::endl;
		  cv::imwrite(filename.str(), frame);
    }
  } while(key != 27); // [ESC]
  return 0;
}