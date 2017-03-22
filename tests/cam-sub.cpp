#include "../include/is.hpp"
#include "../include/msgs/camera.hpp"
#include "../include/theora-decoder.hpp"

#include <opencv2/highgui.hpp>

using namespace std::chrono_literals;

int main(int, char* []) {
  std::string uri = "amqp://localhost";
  auto is = is::connect(uri);

  auto frames = is.subscribe("webcam.frame");
  auto client = is::make_client(is);

  is::TheoraDecoder decoder;

  for (;;) {
    auto req_id = client.request("webcam.get_headers", is::msgpack(""));
    auto reply = client.receive_for(1s, req_id, is::policy::discard_others);
    if (reply != nullptr) {
      auto headers = is::msgpack<std::vector<is::msg::camera::TheoraPacket>>(reply);
      decoder.set_headers(headers);
      break;
    }
  }

  for (;;) {
    auto envelope = is.consume(frames);
    auto packet = is::msgpack<is::msg::camera::TheoraPacket>(envelope);
    auto maybe_frame = decoder.decode(packet);
    is::log::info("{}", is::latency(envelope));
    if (maybe_frame) {
      cv::imshow("webcam", *maybe_frame);
      cv::waitKey(1);
    }
  }
}