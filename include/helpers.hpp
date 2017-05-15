#ifndef __IS_HELPERS_HPP__
#define __IS_HELPERS_HPP__

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <memory>
#include <chrono>
#include <sstream>
#include "logger.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace is {

using namespace std::chrono;
using namespace AmqpClient;

Channel::ptr_t make_channel(std::string const& uri) {
  log::info("Trying to connect to broker at \"{}\"", uri);
  try {
    auto channel = Channel::CreateFromUri(uri);
    log::info("Connection successful");
    return channel;
  } catch (...) {
    log::critical("Failed to establish connection to broker!");
    exit(1);
  }
}

auto time_since_epoch_ns() {
  const boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
  return (boost::posix_time::microsec_clock::universal_time() - epoch).total_nanoseconds();
}

std::string make_uid() {
  return boost::uuids::to_string(boost::uuids::random_generator()());
}

auto latency(Envelope::ptr_t envelope) {
  auto now = system_clock::now().time_since_epoch().count();
  auto diff = nanoseconds(now - envelope->Message()->Timestamp());
  return duration_cast<milliseconds>(diff).count();
}

}  // ::is

#endif  // __IS_HELPERS_HPP__
