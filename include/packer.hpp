#ifndef __IS_PACKER_HPP__
#define __IS_PACKER_HPP__

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <msgpack.hpp>
#include <sstream>

/*
  Reference: https://github.com/msgpack/msgpack-c/wiki/v1_1_cpp_adaptor
*/

namespace is {

using namespace AmqpClient;

template <typename T>
BasicMessage::ptr_t msgpack(T const& data) {
  std::stringstream body;
  msgpack::pack(body, data);
  auto message = BasicMessage::Create(body.str());
  message->ContentEncoding("msgpack");
  return message;
}

template <typename T>
T msgpack(Envelope::ptr_t envelope) {
  auto message = envelope->Message();
  auto body = message->Body();
  msgpack::object_handle handle = msgpack::unpack(body.data(), body.size());
  msgpack::object object = handle.get();
  T data;
  object.convert(data);
  return data;
}

}  // ::is

#endif  // __IS_PACKER_HPP__
