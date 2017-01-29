#include <is/is.hpp>
#include <is/msgs.hpp>
#include <opencv2/highgui.hpp>

int main(int argc, char* argv[]) {
  auto is = is::connect("amqp://guest:guest@localhost:5672");
  auto frames = is.subscribe("webcam.frame");

  cv::VideoCapture webcam(0);
  assert(webcam.isOpened());

  while (1) {
    {  // Publisher code
      cv::Mat frame;
      webcam >> frame;
      is::msg::camera::CompressedImage image;
      image.format = ".png";
      cv::imencode(image.format, frame, image.data);
      is.publish("webcam.frame", is::msgpack(image));
    }
    {  // Consumer code
      auto message = is.consume(frames);
      auto image = is::msgpack<is::msg::camera::CompressedImage>(message);
      cv::Mat frame = cv::imdecode(image.data, CV_LOAD_IMAGE_COLOR);
      cv::imshow("Webcam", frame);
      cv::waitKey(1);
    }
  }
}