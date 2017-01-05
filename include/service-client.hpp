#ifndef __IS_SERVICE_CLIENT_HPP__
#define __IS_SERVICE_CLIENT_HPP__

#include <SimpleAmqpClient/SimpleAmqpClient.h>

namespace is {

class ServiceClient {
  Channel::ptr_t channel;
  int correlation_id;
  std::string rpc_queue;
  std::string rpc_tag;

 public:
  ServiceClient(Channel::ptr_t channel) : channel(channel), correlation_id(0) {
    // queue_name, passive, durable, exclusive, auto_delete
    rpc_queue = channel->DeclareQueue("", false, false, true, true);
    channel->BindQueue(rpc_queue, "amq.topic", rpc_queue);
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
      channel->BasicPublish("amq.topic", route, message, mandatory);
    } else {
      message->ReplyTo(route.substr(pos + 1) + ';' + rpc_queue);
      channel->BasicPublish("amq.topic", route.substr(0, pos), message, mandatory);
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