#ifndef __IS_HELPERS_HPP__
#define __IS_HELPERS_HPP__

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <memory>
#include <sstream>
#include "logger.hpp"

namespace is {

using namespace AmqpClient;

Channel::ptr_t make_channel(std::string const& uri) {
  logger()->info("Trying to connect to broker at \"{}\"", uri);
  try {
    auto&& channel{Channel::CreateFromUri(uri)};
    logger()->info("Connection successful");
    return channel;
  } catch (...) {
    logger()->critical("Failed to establish connection to broker!");
    exit(1);
  }
}

void set_timestamp(BasicMessage::ptr_t message) {
  using namespace std::chrono;
  message->Timestamp(system_clock::now().time_since_epoch().count());
}

auto latency(Envelope::ptr_t envelope) {
  using namespace std::chrono;
  auto now = system_clock::now().time_since_epoch().count();
  auto diff = nanoseconds(now - envelope->Message()->Timestamp());
  return duration_cast<milliseconds>(diff).count();
}

}  // ::is

#endif  // __IS_HELPERS_HPP__
