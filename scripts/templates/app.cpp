#include <is/is.hpp>

int main(int, char* []) {
  std::string uri = "amqp://guest:guest@localhost:5672";
  auto is = is::connect(uri);
}