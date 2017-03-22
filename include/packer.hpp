#ifndef __IS_PACKER_HPP__
#define __IS_PACKER_HPP__

#define MSGPACK_USE_BOOST

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <msgpack.hpp>
#include <sstream>
#include <string>
#include <vector>

#define IS_DEFINE_MSG MSGPACK_DEFINE_ARRAY

/*
  Reference: https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_overview
*/

namespace is {

using namespace AmqpClient;

template <class T>
std::string pack(T&& t) {
  std::stringstream ss;
  msgpack::pack(ss, std::forward<T>(t));
  return ss.str();
}

template <class... Args>
std::string pack(Args&&... args) {
  std::stringstream ss;
  msgpack::pack(ss, std::make_tuple(std::forward<Args>(args)...));
  return ss.str();
}

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
