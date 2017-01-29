#ifndef __IS_CONNECTION_HPP__
#define __IS_CONNECTION_HPP__

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <algorithm>
#include <chrono>
#include <string>
#include <vector>
#include "helpers.hpp"

namespace is {

using namespace AmqpClient;
using namespace std::chrono;

struct Connection {
  Channel::ptr_t channel;
  const std::string exchange;

  Connection(std::string const& uri, std::string const& exchange = "data");
  Connection(Channel::ptr_t channel, std::string const& exchange = "data");

  bool publish(std::string const& topic, BasicMessage::ptr_t message, bool mandatory = false);

  std::string subscribe(std::string const& topic, int queue_size = 1);
  std::string subscribe(std::vector<std::string> const& topics, int queue_size = 0);

  Envelope::ptr_t consume(std::string const& tag);
  template <typename Time>
  Envelope::ptr_t consume_for(std::string const& tag, Time const& timeout);
  std::vector<Envelope::ptr_t> consume_sync(std::vector<std::string> const& tags, int64_t period_ms);
  std::vector<Envelope::ptr_t> consume_sync(std::string const& tag, std::vector<std::string> topics, int64_t period_ms);
};

}  // ::is

#endif  // __IS_CONNECTION_HPP__