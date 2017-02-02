#ifndef __IS_SERVICE_CLIENT_HPP__
#define __IS_SERVICE_CLIENT_HPP__

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <unordered_map>
#include "logger.hpp"

namespace is {

using namespace AmqpClient;
using namespace std::chrono;

struct discard_others_tag {};

namespace policy {
const auto discard_others = discard_others_tag{};
}

class ServiceClient {
  Channel::ptr_t channel;
  const std::string exchange;

  int correlation_id;
  std::string rpc_queue;
  std::string rpc_tag;

 public:
  ServiceClient(Channel::ptr_t channel, std::string const& exchange = "services");

  std::string request(std::string const& route, BasicMessage::ptr_t message);

  template <class Time>
  Envelope::ptr_t receive_for(Time const& timeout);

  template <class Time>
  Envelope::ptr_t receive_for(Time const& timeout, std::string const& id, discard_others_tag);

  template <class Time>
  Envelope::ptr_t receive_until(Time const& deadline);

  template <class Time>
  Envelope::ptr_t receive_until(Time const& deadline, std::string const& id, discard_others_tag);

  template <class Time>
  std::unordered_map<std::string, Envelope::ptr_t> receive_until(Time const& deadline,
                                                                 std::vector<std::string> const& ids,
                                                                 discard_others_tag);
};  // ServiceClient

template <class Time>
Envelope::ptr_t ServiceClient::receive_for(Time const& timeout) {
  int timeout_ms = duration_cast<milliseconds>(timeout).count();
  Envelope::ptr_t envelope;
  channel->BasicConsumeMessage(rpc_tag, envelope, timeout_ms);
  return envelope;
}

template <class Time>
Envelope::ptr_t ServiceClient::receive_for(Time const& timeout, std::string const& id, discard_others_tag) {
  Envelope::ptr_t envelope;
  do {
    envelope = receive_for(timeout);
  } while (envelope != nullptr && envelope->Message()->CorrelationId() != id);
  return envelope;
}

template <class Time>
Envelope::ptr_t ServiceClient::receive_until(Time const& deadline) {
  auto timeout = deadline - system_clock::now();
  Envelope::ptr_t envelope;
  if (duration_cast<milliseconds>(timeout).count() >= 1.0) {
    envelope = receive_for(timeout);
  }
  return envelope;
}

template <class Time>
Envelope::ptr_t ServiceClient::receive_until(Time const& deadline, std::string const& id, discard_others_tag) {
  Envelope::ptr_t envelope;
  do {
    envelope = receive_until(deadline);
  } while (envelope != nullptr && envelope->Message()->CorrelationId() != id);
  return envelope;
}

template <class Time>
std::unordered_map<std::string, Envelope::ptr_t> ServiceClient::receive_until(Time const& deadline,
                                                                              std::vector<std::string> const& ids,
                                                                              discard_others_tag) {
  std::unordered_map<std::string, Envelope::ptr_t> map;
  auto n = ids.size();
  while (n) {
    auto envelope = receive_until(deadline);
    if (envelope != nullptr) {
      auto id = std::find(std::begin(ids), std::end(ids), envelope->Message()->CorrelationId());
      if (id != std::end(ids)) {
        map.emplace(*id, envelope);
        --n;
      }
    } else {
      break;
    }
  };
  return map;
}

}  // ::is

#endif  // __IS_SERVICE_CLIENT_HPP__