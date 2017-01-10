#ifndef __IS_SERVICE_CLIENT_HPP__
#define __IS_SERVICE_CLIENT_HPP__

#include <string>
#include <map>

#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <ev.h>

#include <boost/fiber/all.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "logger.hpp"

namespace is {

struct ServiceClient {
  AMQP::TcpChannel channel;

  boost::uuids::random_generator generator;
  std::map<std::string, boost::fibers::promise<AMQP::Message>> requests;
  std::string reply_queue;

  ServiceClient(Connection& c) : channel(&c.connection) { make_reply_queue(); }

  auto request(std::string const& route, AMQP::Envelope&& envelope) {
    boost::uuids::uuid id = generator();
    auto correlation_id = boost::uuids::to_string(id);
    envelope.setCorrelationID(correlation_id);

    auto&& pos = route.find_first_of(';');
    if (pos == std::string::npos || pos + 1 > route.size()) {
      envelope.setReplyTo(reply_queue);
      channel.publish("amq.topic", route, envelope);
    } else {
      envelope.setReplyTo(route.substr(pos + 1) + ';' + reply_queue);
      channel.publish("amq.topic", route.substr(0, pos), envelope);
    }

    boost::fibers::promise<AMQP::Message> promise;
    auto&& future = promise.get_future();
    requests.emplace(correlation_id, std::move(promise));

    return std::move(future);
  }

  void make_reply_queue() {
    auto&& on_success = [&](const std::string& queue, uint32_t, uint32_t) {
      channel.bindQueue("amq.topic", queue, queue);
      reply_queue = queue;

      auto&& on_received = [&](const AMQP::Message& message, uint64_t, bool) {
        auto it = requests.find(message.correlationID());

        if (it != std::end(requests)) {
          it->second.set_value(message);
          requests.erase(it);
          boost::this_fiber::yield();
        }
      };

      auto&& on_success = [&]() { ev_break(EV_DEFAULT, EVBREAK_ALL); };

      channel.consume(queue, AMQP::noack)
          .onReceived(on_received)
          .onSuccess(on_success);
    };

    channel.declareQueue(AMQP::exclusive + AMQP::autodelete)
        .onSuccess(on_success);

    ev_run(EV_DEFAULT, 0);
  }

};  // ServiceClient

}  // ::is

#endif  // __IS_SERVICE_CLIENT_HPP__