#ifndef __IS_MSG_COMMON_HPP__
#define __IS_MSG_COMMON_HPP__

#include <string>
#include <vector>
#include "../packer.hpp"

namespace is {
namespace msg {
namespace common {

enum class Status { OK, FAILED };

struct Delay {
  int milliseconds;
  MSGPACK_DEFINE_ARRAY(milliseconds);
};

}  // ::camera
}  // ::msg
}  // ::is

// Enum packing must be done on global namespace
MSGPACK_ADD_ENUM(is::msg::common::Status);

#endif  // __IS_MSG_COMMON_HPP__
