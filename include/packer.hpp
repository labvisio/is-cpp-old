#ifndef __IS_PACKER_HPP__
#define __IS_PACKER_HPP__

#include <amqpcpp.h>
#include <msgpack.hpp>
#include <sstream>

/*
  Reference: https://github.com/msgpack/msgpack-c/wiki/v1_1_cpp_adaptor
*/

namespace is {

template <typename T>
AMQP::Envelope msgpack(T const& data) {
  std::stringstream body;
  msgpack::pack(body, data);
  auto envelope = AMQP::Envelope{body.str()};
  envelope.setContentEncoding("msgpack");
  return envelope;
}

template <typename T>
T msgpack(AMQP::Message const& message) {
  msgpack::object_handle handle = msgpack::unpack(message.body(), message.bodySize());
  msgpack::object object = handle.get();
  T data;
  object.convert(data);
  return data;
}

}  // ::is

#endif  // __IS_PACKER_HPP__