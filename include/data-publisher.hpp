#ifndef __IS_DATA_PUBLISHER_HPP__
#define __IS_DATA_PUBLISHER_HPP__

#include "connection.hpp"
#include "event-watcher.hpp"

#include <string>
#include <functional>
#include <unordered_map>

namespace is {

struct DataPublisher {
  using Message = AmqpClient::BasicMessage::ptr_t;

  struct Generator {
    bool has_consumers;
    std::function<Message()> generator;
  };

  Connection is;
  EventWatcher watcher;
  std::unordered_map<std::string, Generator> generators;

  DataPublisher(Connection connection)
      : is(connection), watcher(connection, "binding.created") {}

  void add(std::string const& topic, std::function<Message()>&& generator) {
    generators.emplace(topic, Generator{.has_consumers = true, .generator = generator});
  }

  int publish() {
    int n = 0;
    for (auto&& key_value : generators) {
      auto&& topic = key_value.first;
      auto& has_consumers = key_value.second.has_consumers;
      auto&& generator = key_value.second.generator;
      if (has_consumers) {
        has_consumers = is.publish(topic, generator(), "data", true);
      }
      n = has_consumers ? n + 1 : n;
    };

    watcher.watch_for(1ms, [&](auto table) {
      auto&& topic = table["routing_key"].GetString();
      auto&& key_value = generators.find(topic);
      if (key_value != generators.end()) {
        key_value->second.has_consumers = true;
        ++n;
      }
    });

    return n;
  }
}; // ::EventWatcher

} // ::is

#endif  // __IS_DATA_PUBLISHER_HPP__