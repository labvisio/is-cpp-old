#ifndef __IS_HPP__
#define __IS_HPP__

#include <thread>
#include "connection.hpp"
#include "helpers.hpp"
#include "packer.hpp"
#include "service-client.hpp"
#include "service-provider.hpp"

namespace is {

Connection connect(std::string const& uri) {
  return {make_channel(uri)};
}

std::thread advertise(std::string const& uri, std::string const& name, std::vector<service_t> const& services) {
  auto thread = std::thread([=]() {
    auto&& channel = make_channel(uri);
    ServiceProvider provider(name, channel);
    for (auto& service : services) {
      auto binding = name + "." + service.name;
      auto handle = service.handle;
      provider.expose(binding, handle);
    }

    provider.listen();
  });

  return thread;
}

ServiceClient make_client(const Connection& c) {
  return ServiceClient(c.channel);
}

}  // ::is

#endif  // __IS_HPP__
