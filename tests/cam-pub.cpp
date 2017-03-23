#include "../include/is.hpp"
#include "../include/msgs/camera.hpp"
#include "../include/theora-encoder.hpp"

#include <opencv2/highgui.hpp>

int main(int, char* []) {
  std::string uri = "amqp://localhost";
  auto is = is::connect(uri);

  cv::VideoCapture webcam(0);
  assert(webcam.isOpened());
  webcam.set(CV_CAP_PROP_FPS, 30);

  is::TheoraEncoder encoder;

  std::thread thread([uri, &encoder]() {
    is::ServiceProvider service("webcam", is::make_channel(uri));
    service.expose("get_headers", [&encoder](auto) {
      auto headers = encoder.get_headers();
      return is::msgpack(headers);  // get_headers is thread safe
    });
    service.listen();
  });

  for (;;) {
    cv::Mat frame;
    webcam >> frame;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    for (auto&& packet : encoder.encode(frame)) {
      auto message = is::msgpack(packet);
      message->Timestamp(now);
      is.publish("webcam.frame", is::msgpack(packet));
    }
  }
}