#include "../include/is.hpp"

is::Reply increment(is::Request req) {
  auto value = is::msgpack<int>(req);
  return is::msgpack(value + 1);
}

using namespace std::chrono_literals;

int main(int , char* []) {
  std::string uri = "amqp://guest:guest@localhost:5672";
  std::thread thread([=] () {
    is::ServiceProvider service("math", is::make_channel(uri));
    service.expose("increment", increment);
    service.listen();
  });

  auto is = is::connect(uri);
  auto client = is::make_client(is);
  int value = 0;
  for (;;) {
    auto req_id = client.request("math.increment", is::msgpack(value));
    auto reply = client.receive_for(1s, req_id, is::policy::discard_others);

    if (reply == nullptr) {
      is::log::error("Request {} timeout!", req_id);
    } else {
      value = is::msgpack<int>(reply);
      is::log::info("Reply: {}", value);
    }

    std::this_thread::sleep_for(1s);
  }

}