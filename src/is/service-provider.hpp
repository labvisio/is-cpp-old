#ifndef __IS_SERVICE_PROVIDER_HPP__
#define __IS_SERVICE_PROVIDER_HPP__

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <atomic>
#include <exception>
#include <string>
#include <unordered_map>
#include "logger.hpp"

namespace is {

using namespace AmqpClient;
using namespace std::chrono;
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
  ServiceProvider(std::string const& name, Channel::ptr_t const& channel, std::string const& exchange = "services");

  void expose(std::string const& binding, service_handle_t service);

  void listen();
};

}  // ::is

#endif  // __IS_SERVICE_PROVIDER_HPP__