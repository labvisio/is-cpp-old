#include "is/service-client.hpp"

namespace is {

using namespace AmqpClient;
using namespace std::chrono;

ServiceClient::ServiceClient(Channel::ptr_t channel, std::string const& exchange)
    : channel(channel), exchange(exchange), correlation_id(0) {
  // passive durable auto_delete
  channel->DeclareExchange(exchange, Channel::EXCHANGE_TYPE_TOPIC, false, false, false);
  // queue_name, passive, durable, exclusive, auto_delete
  rpc_queue = channel->DeclareQueue("", false, false, true, true);
  channel->BindQueue(rpc_queue, exchange, rpc_queue);
  // no_local, no_ack, exclusive
  rpc_tag = channel->BasicConsume(rpc_queue, "", true, true, true);
}

std::string ServiceClient::request(std::string const& route, BasicMessage::ptr_t message) {
  auto id = std::to_string(correlation_id);
  message->CorrelationId(id);
  ++correlation_id;

  bool mandatory{true};  // fail fast if no service provider exists on the
                         // specified route

  try {
    auto&& pos = route.find_first_of(';');
    if (pos == std::string::npos || pos + 1 > route.size()) {
      message->ReplyTo(rpc_queue);
      channel->BasicPublish(exchange, route, message, mandatory);
    } else {
      message->ReplyTo(route.substr(pos + 1) + ';' + rpc_queue);
      channel->BasicPublish(exchange, route.substr(0, pos), message, mandatory);
    }
  } catch (MessageReturnedException) {
    log::warn("No route for {}", route);
  }

  return id;
}

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