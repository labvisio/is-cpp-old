#include <boost/program_options.hpp>
#include <iostream>
#include <chrono>
#include <opencv2/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "../../include/gateways/camera.hpp"
#include "../../include/is.hpp"
#include "../../include/msgs/common.hpp"
#include "../../include/msgs/camera.hpp"

using namespace std::chrono_literals;
using namespace is::msg::camera;
using namespace is::msg::common;
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

  auto image_type = [&] () { return (imtype_str == "RGB") ? ImageType::RGB : ImageType::GRAY; }();
  client.request(entity + ".set_fps", is::msgpack(fps));
  client.request(entity + ".set_resolution", is::msgpack(resolution));
  client.request(entity + ".set_image_type", is::msgpack(image_type));
  while (client.receive(1s) != nullptr);

  auto frames = is.subscribe({entity + ".frame"});

  while (1) {
    cv::Mat frame;
    auto image_message = is.consume(frames);
    auto image = is::msgpack<is::msg::camera::CompressedImage>(image_message);
    Pattern pattern;

      auto req_id = client.request("pattern.find", image_message->Message());
      auto reply = client.receive(1s);
      frame = cv::imdecode(image.data, CV_LOAD_IMAGE_COLOR);
      if (reply != nullptr) {
        if (reply->Message()->CorrelationId() == req_id) {
          pattern = is::msgpack<Pattern>(reply);
          if (pattern.found) {
            bool first = true;
            for (auto& p : pattern.points) {
              cv::circle(frame, cv::Point2d(p.x, p.y), 3, first ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0), 3);
              first = false;
            }
          }
        } else {
          is::logger()->warn("Invalid correlation id.");
       }
      } else {
        is::logger()->warn("Empty message.");
      }

      cv::imshow("Pattern", frame);
      cv::waitKey(1);
    }
    return 0;
  }