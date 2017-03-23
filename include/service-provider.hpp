#ifndef __IS_SERVICE_PROVIDER_HPP__
#define __IS_SERVICE_PROVIDER_HPP__

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <atomic>
#include <exception>
#include <string>
#include <unordered_map>
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
  const std::string name;
  Channel::ptr_t channel;
  const std::string exchange;

  std::unordered_map<std::string, service_handle_t> map;

 public:
  ServiceProvider(std::string const& name, Channel::ptr_t const& channel,
                  std::string const& exchange = "services")
      : name(name), channel(channel), exchange(exchange) {
    // passive durable auto_delete
    channel->DeclareExchange(exchange, Channel::EXCHANGE_TYPE_TOPIC, false, false, false);
    // passive, durable, exclusive, auto_delete
    Table arguments{{TableKey("x-expires"), TableValue(30000)},
                    {TableKey("x-max-length"), TableValue(32)}};
    channel->DeclareQueue(name, false, false, false, false, arguments);
  }

  void expose(std::string const& binding, service_handle_t service) {
    auto topic = name + '.' + binding;
    channel->BindQueue(name, exchange, topic);
    map.emplace(topic, service);
  }

  void listen() {
    // no_local, no_ack, exclusive
    auto tag = channel->BasicConsume(name, "", true, false, false);

    log::info("Listening for service requests");

    while (1) {
      auto request = channel->BasicConsumeMessage(tag);
      auto service = map.find(request->RoutingKey());
      if (service != map.end()) {
        log::info("New request \"{}\"", request->RoutingKey());

        try {
          auto reply = service->second(request);

          if (request->Message()->CorrelationIdIsSet()) {
            reply->CorrelationId(request->Message()->CorrelationId());
          }

          if (request->Message()->ReplyToIsSet()) {
            auto mandatory = true;
            auto route = request->Message()->ReplyTo();

            auto pos = route.find_first_of(';');
            if (pos == std::string::npos || pos + 1 > route.size()) {
              channel->BasicPublish(exchange, route, reply, mandatory);
            } else {
              reply->ReplyTo(route.substr(pos + 1));
              channel->BasicPublish(exchange, route.substr(0, pos), reply, mandatory);
            }
          }
        } catch (std::exception const& e) {
          log::error("Service \"{}\" throwed an exception! \n\t@reason: \"{}\"",
                     request->RoutingKey(), e.what());
        }
      } else {
        log::warn("Invalid service requested \"{}\"", request->RoutingKey());
      }

      channel->BasicAck(request);
    }
  }
}; // ::ServiceProvider

}  // ::is

#endif  // __IS_SERVICE_PROVIDER_HPP__