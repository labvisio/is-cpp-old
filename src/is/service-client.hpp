#ifndef __IS_SERVICE_CLIENT_HPP__
#define __IS_SERVICE_CLIENT_HPP__

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <unordered_map>
#include "logger.hpp"

namespace is {

using namespace AmqpClient;
using namespace std::chrono;

struct discard_others_tag {};

namespace policy {
const auto discard_others = discard_others_tag{};
}

class ServiceClient {
  Channel::ptr_t channel;
  const std::string exchange;

  int correlation_id;
  std::string rpc_queue;
  std::string rpc_tag;

 public:
  ServiceClient(Channel::ptr_t channel, std::string const& exchange = "services");

  std::string request(std::string const& route, BasicMessage::ptr_t message);

  template <class Time>
  Envelope::ptr_t receive_for(Time const& timeout);

  template <class Time>
  Envelope::ptr_t receive_for(Time const& timeout, std::string const& id, discard_others_tag);

  template <class Time>
  Envelope::ptr_t receive_until(Time const& deadline);

  template <class Time>
  Envelope::ptr_t receive_until(Time const& deadline, std::string const& id, discard_others_tag);

  template <class Time>
  std::unordered_map<std::string, Envelope::ptr_t> receive_until(Time const& deadline,
                                                                 std::vector<std::string> const& ids,
                                                                 discard_others_tag);
};  // ServiceClient

}  // ::is

#endif  // __IS_SERVICE_CLIENT_HPP__