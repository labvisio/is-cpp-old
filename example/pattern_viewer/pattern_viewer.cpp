#include <armadillo>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <is/gateways/camera.hpp>
#include <is/is.hpp>
#include <is/msgs/camera.hpp>
#include <is/msgs/common.hpp>
#include <is/msgs/robot.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace arma;
using namespace std::chrono_literals;
using namespace is::msg::camera;
using namespace is::msg::robot;
using namespace is::msg::common;
namespace po = boost::program_options;

namespace fifi {

template <class Time>
AmqpClient::Envelope::ptr_t wait(is::ServiceClient& client, std::string const& id, Time duration) {
  auto until = std::chrono::system_clock::now() + duration;

  while (1) {
    auto block = until - std::chrono::system_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(block).count() < 0.0) {
      return nullptr;
    }

    auto reply = client.receive(block);
    if (reply == nullptr) {
      is::logger()->warn("Pattern: Empty message.");
    } else if (reply->Message()->CorrelationId() != id) {
      is::logger()->warn("Pattern: Invalid correlation id.[{},{}]", id, reply->Message()->CorrelationId());
    } else {
      return reply;
    }
  }
}
}

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

  auto image_type = [&]() { return (imtype_str == "RGB") ? ImageType::RGB : ImageType::GRAY; }();
  sample_rate = {fps};

  client.request(entity + ".set_sample_rate", is::msgpack(sample_rate));
  client.request(entity + ".set_resolution", is::msgpack(resolution));
  client.request(entity + ".set_image_type", is::msgpack(image_type));

  while (client.receive(1s) != nullptr) {
  }

  auto frames = is.subscribe({entity + ".frame"});

  while (1) {
    cv::Mat frame;
    auto image_message = is.consume(frames);
    auto image = is::msgpack<is::msg::camera::CompressedImage>(image_message);
    frame = cv::imdecode(image.data, CV_LOAD_IMAGE_COLOR);

    AmqpClient::Envelope::ptr_t reply;
    do {
      auto req_id = client.request("pattern.find", image_message->Message());
      reply = fifi::wait(client, req_id, 1s);
    } while (reply == nullptr);

    Pattern pattern = is::msgpack<Pattern>(reply);
    pattern.frame = entity;

    if (!pattern.points.empty()) {
      FrameChangeRequest fc_request{{pattern, {{}, "ptgrey.1"}}, 650.0};

      do {
        auto req_id = client.request("frame_change.camera_to_world", is::msgpack<FrameChangeRequest>(fc_request));
        reply = fifi::wait(client, req_id, 1s);
      } while (reply == nullptr);

      auto points3d = is::msgpack<std::vector<Point3d>>(reply);

      std::cout << points3d.front().x << ',' << points3d.front().y << ',' << points3d.front().z << '\n';

      do {
        auto req_id = client.request("pattern.get_pose", is::msgpack(points3d));
        reply = fifi::wait(client, req_id, 1s);
      } while (reply == nullptr);

      Odometry odo = is::msgpack<Odometry>(reply);

      cv::putText(frame, std::to_string(odo.x) + ";" + std::to_string(odo.y) + ";" + std::to_string(odo.th),
                  cv::Point2f(50, 100), 1, 4, cv::Scalar(0, 255, 0), 6);
      bool first = true;
      for (auto& p : pattern.points) {
        cv::circle(frame, cv::Point2d(p.x, p.y), 3, first ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0), 3);
        first = false;
      }
    }

    cv::imshow("Pattern", frame);
    cv::waitKey(1);
  }
  return 0;
}