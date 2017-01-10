#ifndef __IS_SERVICE_PROVIDER_HPP__
#define __IS_SERVICE_PROVIDER_HPP__

#include <ev.h>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>

#include <string>
#include <map>

#include "logger.hpp"

namespace is {

using Request = AMQP::Message;
using Reply = AMQP::Envelope;
using ServiceHandle = std::function<Reply(Request)>;

struct ServiceProvider {
  AMQP::TcpChannel channel;

  std::map<std::string, ServiceHandle> services;
  std::string service_queue;

  ServiceProvider(Connection& c, std::string const& entity)
      : channel(&c.connection) {
    make_request_queue(entity);
  }

  void make_request_queue(std::string const& name) {
    service_queue = name;

    AMQP::Table arguments;
    arguments["x-expires"] = 30000;

    auto&& on_success = [&](std::string const& queue, uint32_t, uint32_t) {

      auto&& on_received = [&](AMQP::Message const& request, uint64_t tag,
                               bool) {
        logger()->info("New service request \"{}\" \"{}\"",
                       request.routingKey(), request.correlationID());

        auto service = services.find(request.routingKey());

        if (service != std::end(services)) {
          auto response = service->second(request);
          response.setCorrelationID(request.correlationID());

          auto&& route = request.replyTo();
          auto&& pos = route.find_first_of(';');
          if (pos == std::string::npos || pos + 1 > route.size()) {
            channel.publish("amq.topic", route, response);
          } else {
            response.setReplyTo(route.substr(pos + 1));
            channel.publish("amq.topic", route.substr(0, pos), response);
          }
        } else {
          logger()->warn("Invalid service requested \"{}\"",
                         request.routingKey());
        }

        channel.ack(tag);
      };

      auto&& on_success = [&]() { ev_break(EV_DEFAULT, EVBREAK_ALL); };
      channel.consume(queue).onReceived(on_received).onSuccess(on_success);
    };

    channel.declareQueue(name, AMQP::autodelete, arguments)
        .onSuccess(on_success);
    ev_run(EV_DEFAULT, 0);
  }

  void advertise(std::string const& name, ServiceHandle&& handle) {
    auto key = service_queue + "." + name;
    channel.bindQueue("amq.topic", service_queue, key);
    services.emplace(key, handle);
    logger()->info("Providing new service on topic \"{}\"", name);
  }

};  // ServiceProvider

}  // ::is

#endif  // __IS_SERVICE_PROVIDER_HPP__