#ifndef __IS_SERVICE_CLIENT_HPP__
#define __IS_SERVICE_CLIENT_HPP__

#include <SimpleAmqpClient/SimpleAmqpClient.h>

namespace is {

class ServiceClient {
  Channel::ptr_t channel;
  const std::string exchange;

  int correlation_id;
  std::string rpc_queue;
  std::string rpc_tag;

 public:
  ServiceClient(Channel::ptr_t channel, std::string const& exchange = "services")
      : channel(channel), exchange(exchange), correlation_id(0) {
    // passive durable auto_delete 
    channel->DeclareExchange(exchange, Channel::EXCHANGE_TYPE_TOPIC, false, false, false);
    // queue_name, passive, durable, exclusive, auto_delete
    rpc_queue = channel->DeclareQueue("", false, false, true, true);
    channel->BindQueue(rpc_queue, exchange, rpc_queue);
    // no_local, no_ack, exclusive
    rpc_tag = channel->BasicConsume(rpc_queue, "", true, true, true);
  }

  std::string request(std::string const& route, BasicMessage::ptr_t message) {
    auto id = std::to_string(correlation_id);
    message->CorrelationId(id);
    ++correlation_id;

    bool mandatory{true};  // fail fast if no service provider exists on the
                           // specified route

    auto&& pos = route.find_first_of(';');
    if (pos == std::string::npos || pos + 1 > route.size()) {
      message->ReplyTo(rpc_queue);
      channel->BasicPublish(exchange, route, message, mandatory);
    } else {
      message->ReplyTo(route.substr(pos + 1) + ';' + rpc_queue);
      channel->BasicPublish(exchange, route.substr(0, pos), message, mandatory);
    }

    return id;
  }

  template <typename Time>
  Envelope::ptr_t receive(Time const& timeout) {
    using namespace std::chrono;
    int timeout_ms = duration_cast<milliseconds>(timeout).count();
    Envelope::ptr_t envelope;
    channel->BasicConsumeMessage(rpc_tag, envelope, timeout_ms);
    return envelope;
  }

};  // ServiceClient

}  // ::is

#endif  // __IS_SERVICE_CLIENT_HPP__