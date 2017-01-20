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
using namespace std::chrono;
namespace po = boost::program_options;

enum class ControllerState { CONSUME_IMAGE, REQUEST_PATTERN, REQUEST_POSE, REQUEST_CONTROL_ACTION, DEADLINE_EXCEEDED };

enum class ViewState { REQUEST_POSE, RECEIVE_POSE, WAITING };

namespace fifi {

template <class Time>
AmqpClient::Envelope::ptr_t wait(is::ServiceClient& client, std::string const& id, Time deadline) {
  while (1) {
    auto block = deadline - system_clock::now();
    if (duration_cast<milliseconds>(block).count() < 0.0) {
      return nullptr;
    }

    auto reply = client.receive(block);
    if (reply == nullptr) {
      // is::logger()->warn("Empty message.");
    } else if (reply->Message()->CorrelationId() != id) {
      // is::logger()->warn("Invalid correlation id.[{},{}]", id, reply->Message()->CorrelationId());
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
  auto view_client = is::make_client(is);
  auto controller_client = is::make_client(is);

  auto image_type = [&]() { return (imtype_str == "RGB") ? ImageType::RGB : ImageType::GRAY; }();
  sample_rate = {fps};

  controller_client.request(entity + ".set_sample_rate", is::msgpack(sample_rate));
  controller_client.request(entity + ".set_resolution", is::msgpack(resolution));
  controller_client.request(entity + ".set_image_type", is::msgpack(image_type));

  while (controller_client.receive(1s) != nullptr) {
  }

  auto frames = is.subscribe({entity + ".frame"});

  time_point<system_clock> deadline;
  ControllerState controller_state = ControllerState::CONSUME_IMAGE;
  CompressedImage image;
  cv::Mat frame;
  Pattern pattern;
  FinalPosition action;
  action.desired = {0.0, 0.0, 0.0};
  action.max_vel_x = 250.0;
  action.max_vel_y = 250.0;
  action.gain_x = 250.0 / 300.0;
  action.gain_y = 250.0 / 300.0;
  action.center_offset = 200.0;

  std::vector<Point2d> desired_points;
  std::string view_request_id;
  ViewState view_state = ViewState::WAITING;

  while (1) {
    switch (controller_state) {
      case ControllerState::CONSUME_IMAGE: {
        auto msg = is.consume(frames);
        deadline = system_clock::now() + 1s;
        image = is::msgpack<CompressedImage>(msg);
        frame = cv::imdecode(image.data, CV_LOAD_IMAGE_COLOR);
        controller_state = ControllerState::REQUEST_PATTERN;

        break;
      }

      case ControllerState::REQUEST_PATTERN: {
        is::logger()->info("[Request Pattern] Deadline {}",
                           duration_cast<milliseconds>(deadline - system_clock::now()).count());

        auto req_id = controller_client.request("pattern.find", is::msgpack(image));
        auto reply = fifi::wait(controller_client, req_id, deadline);

        if (reply == nullptr) {
          controller_state = ControllerState::DEADLINE_EXCEEDED;
        } else {
          pattern = is::msgpack<Pattern>(reply);
          if (!pattern.found) {
            controller_state = ControllerState::CONSUME_IMAGE;
          } else {
            controller_state = ControllerState::REQUEST_POSE;
          }
        }
        break;
      }

      case ControllerState::REQUEST_POSE: {
        is::logger()->info("[Request Pose] Deadline {}",
                           duration_cast<milliseconds>(deadline - system_clock::now()).count());

        auto req_id = controller_client.request("pattern.get_pose", is::msgpack(pattern));
        auto reply = fifi::wait(controller_client, req_id, deadline);
        if (reply == nullptr) {
          controller_state = ControllerState::DEADLINE_EXCEEDED;
        } else {
          action.current = is::msgpack<Odometry>(reply);
          controller_state = ControllerState::REQUEST_CONTROL_ACTION;
        }
        break;
      }

      case ControllerState::REQUEST_CONTROL_ACTION: {
        is::logger()->info("[Control Action] Deadline {}",
                           duration_cast<milliseconds>(deadline - system_clock::now()).count());
        // is::logger()->info("[Control Action] x:{}, y:{}, th:{}", action.current.x, action.current.y,
        // action.current.th);

        double error = std::sqrt(std::pow(action.desired.x - action.current.x, 2.0) +
                                 std::pow(action.desired.y - action.current.y, 2.0));
        std::string req_id;
        if (error < 100.0) {
          req_id = controller_client.request("pioneer.0.set_velocities", is::msgpack<Velocities>({0.0, 0.0}));
        } else {
          req_id = controller_client.request("controller.final_position;pioneer.0.set_velocities",
                                             is::msgpack<FinalPosition>(action));
        }

        auto reply = fifi::wait(controller_client, req_id, deadline);

        if (reply == nullptr) {
          controller_state = ControllerState::DEADLINE_EXCEEDED;
        } else {
          controller_state = ControllerState::CONSUME_IMAGE;
        }
        break;
      }

      case ControllerState::DEADLINE_EXCEEDED: {
        is::logger()->warn("Deadline Lost");
        controller_state = ControllerState::CONSUME_IMAGE;
        break;
      }
    }

    cv::putText(frame, std::to_string(action.current.x) + ";" + std::to_string(action.current.y) + ";" +
                           std::to_string(action.current.th),
                cv::Point2f(50, 100), 1, 4, cv::Scalar(0, 255, 0), 6);

    bool first = true;
    for (auto& p : pattern.points) {
      cv::circle(frame, cv::Point2d(p.x, p.y), 3, first ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0), 3);
      first = false;
    }

    switch (view_state) {
      case ViewState::WAITING: {
        if (!desired_points.empty()) {
          view_state = ViewState::REQUEST_POSE;
        }
        break;
      }

      case ViewState::REQUEST_POSE: {
        auto point = desired_points.back();
        desired_points.clear();
        view_request_id = view_client.request("pattern.image_to_world", is::msgpack(point));
        view_state = ViewState::RECEIVE_POSE;
        break;
      }

      case ViewState::RECEIVE_POSE: {
        auto reply = view_client.receive(1ms);
        if (reply != nullptr && reply->Message()->CorrelationId() == view_request_id) {
          action.desired = is::msgpack<Odometry>(reply);
          view_state = ViewState::WAITING;
          is::logger()->info("New setpoint x:{}, y:{}", action.desired.x, action.desired.y);
        }
        break;
      }
    }

    cv::imshow("robot", frame);
    cv::setMouseCallback("robot",
                         [](int event, int x, int y, int, void* params) {
                           if (event == 4) {  // CLICK_RELEASE
                             auto points = (std::vector<Point2d>*)params;
                             is::logger()->error("clicked {} {}", x, y);
                             auto point = Point2d{static_cast<double>(x), static_cast<double>(y)};
                             points->push_back(point);
                           }
                         },
                         &desired_points);

    cv::waitKey(1);
  }
  return 0;
}