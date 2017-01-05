#ifndef __IS_SERVICE_PROVIDER_HPP__
#define __IS_SERVICE_PROVIDER_HPP__

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <atomic>
#include <string>
#include "logger.hpp"
namespace is {

using Request = Envelope::ptr_t;
using Reply = BasicMessage::ptr_t;
using service_handle_t = std::function<Reply(Request)>;

struct service_t {
  std::string name;
  service_handle_t handle;
};

class ServiceProvider {
  std::string name;
  Channel::ptr_t channel;
  std::unordered_map<std::string, service_handle_t> map;

 public:
  ServiceProvider(std::string const& name, Channel::ptr_t const& channel) : name(name), channel(channel) {
    // passive, durable, exclusive, auto_delete
    Table arguments{{TableKey("x-expires"), TableValue(30000)}};
    channel->DeclareQueue(name, false, false, false, false, arguments);
  }

  void expose(std::string const& binding, service_handle_t service) {
    logger()->info("({}) Exposing new service on topic \"{}\"", name, binding);
    channel->BindQueue(name, "amq.topic", binding);
    map.emplace(binding, service);
  }

  bool request_is_valid(Envelope::ptr_t request) {
    auto&& valid = request->Message()->CorrelationIdIsSet() && request->Message()->ReplyToIsSet();
    if (!valid) {
      logger()->warn("({}) Malformed request @\"{}\"", name, request->RoutingKey());
    }
    return valid;
  }

  void listen() {
    // no_local, no_ack, exclusive
    auto tag = channel->BasicConsume(name, "", true, false, false);

    logger()->info("({}) Listening incoming service requests", name);
    while (1) {
      Envelope::ptr_t request;
      do {
        request = channel->BasicConsumeMessage(tag);
      } while (!request_is_valid(request));

      auto service = map.find(request->RoutingKey());

      if (service != map.end()) {
        logger()->info("({}) New service request \"{}\"", name, request->RoutingKey());
        auto response = service->second(request);
        response->CorrelationId(request->Message()->CorrelationId());
        logger()->info("({}) Done processing \"{}\" id:\"{}\"", name, request->RoutingKey(), response->CorrelationId());

        auto&& route = request->Message()->ReplyTo();
        auto&& pos = route.find_first_of(';');
        if (pos == std::string::npos || pos + 1 > route.size()) {
          channel->BasicPublish("amq.topic", route, response);
        } else {
          response->ReplyTo(route.substr(pos + 1));
          channel->BasicPublish("amq.topic", route.substr(0, pos), response);
        }
      } else {
        logger()->warn("({}) Invalid service requested \"{}\"", name, request->RoutingKey());
      }

      channel->BasicAck(request);
    }
  }
};

}  // ::is

#endif  // __IS_SERVICE_PROVIDER_HPP__