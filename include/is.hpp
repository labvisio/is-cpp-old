#ifndef __IS_HPP__
#define __IS_HPP__

#include "logger.hpp"
#include "packer.hpp"
#include "timer.hpp"
#include "connection.hpp"
#include "service-client.hpp"
#include "service-provider.hpp"

namespace is {

void run() {
  ev_run(EV_DEFAULT, 0);
}

}  // ::is

#endif  // __IS_HPP__
