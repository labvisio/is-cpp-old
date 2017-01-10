#ifndef __IS_CONNECTION_HPP__
#define __IS_CONNECTION_HPP__

#include <string>
#include <vector>
#include <memory>

#include <ev.h>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>

#include <boost/fiber/all.hpp>

#include "logger.hpp"

namespace is {

using Channel = boost::fibers::unbounded_channel<AMQP::Message>;

struct Connection : AMQP::LibEvHandler {
  AMQP::TcpConnection connection;
  AMQP::TcpChannel channel;

  void onConnected(AMQP::TcpConnection* connection) override {
    logger()->info("Connected");
  }

  void onError(AMQP::TcpConnection* connection, const char* message) override {
    logger()->error("Connection error: \"{}\"", message);
    exit(-1);
  }

  void onClosed(AMQP::TcpConnection* connection) override {
    logger()->error("Connection closed...");
    exit(-1);
  }

  Connection(std::string const& uri)
      : AMQP::LibEvHandler(EV_DEFAULT),
        connection(this, AMQP::Address(uri)),
        channel(&connection) {
    bool ready = false;
    channel.onError([](const char* error) {
      logger()->error("Channel error \"{}\"", error);
    });

    channel.onReady([&ready]() {
      ready = true;
      ev_break(EV_DEFAULT, EVBREAK_ALL);
    });

    ev_timer timer;
    ev_timer_init(&timer, [](struct ev_loop*, ev_timer*,
                             int) { ev_break(EV_DEFAULT, EVBREAK_ALL); },
                  2.0, 0.0);
    ev_timer_start(EV_DEFAULT, &timer);

    logger()->info("Trying to connect to \"{}\"", uri);
    ev_run(EV_DEFAULT, 0);

    ev_timer_stop(EV_DEFAULT, &timer);

    if (!ready) {
      logger()->error("Connection failed...");
      exit(-1);
    }
  }

  void publish(std::string const& topic, AMQP::Envelope const& envelope) {
    channel.publish("amq.topic", topic, envelope);
  }

  std::shared_ptr<Channel> subscribe(std::string const& topic) {
    AMQP::Table arguments;
    arguments["x-max-length"] = 1;

    auto c = std::make_shared<Channel>();
    auto on_success = [this, topic, c](std::string const& queue, uint32_t,
                                       uint32_t) {
      logger()->info("Subscribing to \"{}\" on queue \"{}\"", topic, queue);
      channel.bindQueue("amq.topic", queue, topic);
      channel.consume(queue, AMQP::noack)
          .onReceived([this, c](AMQP::Message const& message, uint64_t, bool) {
            c->push(message);
            boost::this_fiber::yield();
          });
    };

    channel.declareQueue(AMQP::exclusive + AMQP::autodelete, arguments)
        .onSuccess(on_success);
    
    return c;
  }

  template <typename Fn, typename... Args>
  void subscribe(std::string const& topic, Fn fn, Args&&... args) {
    auto queue = subscribe(topic);

    boost::fibers::fiber fiber(
        [this, queue](Fn&& fn, Args&&... args) {
          while (1) {
            fn(queue, std::forward<Args>(args)...);
          }
        },
        std::forward<Fn>(fn), std::forward<Args>(args)...);

    fiber.detach();
  }

};  // Connection

}  // ::is

#endif  // __IS_CONNECTION_HPP__