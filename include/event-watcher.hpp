#ifndef __IS_EVENT_WATCHER_HPP__
#define __IS_EVENT_WATCHER_HPP__

#include "connection.hpp"

namespace is {

struct EventWatcher {
  using Table = AmqpClient::Table;

  Connection is;
  QueueInfo events;

  EventWatcher(Connection connection, std::string const& topic,
               std::string const& exchange = "amq.rabbitmq.event")
      : is(connection), events(is.subscribe(topic, exchange, 32)) {}

  ~EventWatcher() { is.unsubscribe(events); }

  template <typename Time>
  void watch_for(Time const& timeout, std::function<void(Table)> on_event) {
    auto event = is.consume_for(events, timeout);
    if (event != nullptr && event->Message()->HeaderTableIsSet()) {
      on_event(event->Message()->HeaderTable());
    }
  }
};  //:: EventWatcher

}  // ::is

#endif  // __IS_EVENT_WATCHER_HPP__