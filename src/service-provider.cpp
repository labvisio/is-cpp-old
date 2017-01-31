#include "is/service-provider.hpp"

namespace is {

using namespace AmqpClient;
using namespace std::chrono;

ServiceProvider::ServiceProvider(std::string const& name, Channel::ptr_t const& channel, std::string const& exchange)
    : name(name), channel(channel), exchange(exchange) {
  // passive durable auto_delete
  channel->DeclareExchange(exchange, Channel::EXCHANGE_TYPE_TOPIC, false, false, false);
  // passive, durable, exclusive, auto_delete
  Table arguments{{TableKey("x-expires"), TableValue(30000)}};
  channel->DeclareQueue(name, false, false, false, false, arguments);
}

void ServiceProvider::expose(std::string const& binding, service_handle_t service) {
  log::info("Exposing service \"{}\"", binding);
  channel->BindQueue(name, exchange, binding);
  map.emplace(binding, service);
}

void ServiceProvider::listen() {
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
        log::error("Service \"{}\" throwed an exception! \n\t@reason: \"{}\"", request->RoutingKey(), e.what());
      }
    } else {
      log::warn("Invalid service requested \"{}\"", request->RoutingKey());
    }

    channel->BasicAck(request);
  }
}

}  // ::is