#ifndef __IS_MSG_COMMON_HPP__
#define __IS_MSG_COMMON_HPP__

#include <chrono>
#include "../packer.hpp"
#include "../helpers.hpp"

namespace is {
namespace msg {
namespace common {

using namespace std::chrono;

struct Status {
  std::string value;
  std::string why;
  IS_DEFINE_MSG(value, why);
};

namespace status {
const auto ok = Status{"ok", ""};
const auto error = [](auto const& why) { return Status{"error", why}; };
}

struct Delay {
  unsigned int milliseconds;
  IS_DEFINE_MSG(milliseconds);
};

struct Timestamp {
  uint64_t nanoseconds = time_since_epoch_ns();  // time since machine epoch in nanoseconds
  IS_DEFINE_MSG(nanoseconds);
};

struct SamplingRate {
  boost::optional<double> rate = boost::none;          // [Hz]
  boost::optional<unsigned int> period = boost::none;  // [ms]
  IS_DEFINE_MSG(rate, period);
};

struct EntityList {
  std::vector<std::string> list;
  IS_DEFINE_MSG(list);
};

}  // ::common
}  // ::msg
}  // ::is

#endif  // __IS_MSG_COMMON_HPP__
