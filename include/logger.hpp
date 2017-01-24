#ifndef __IS_LOGGER_HPP__
#define __IS_LOGGER_HPP__

#include <spdlog/spdlog.h>

namespace is {

struct Logger {
  std::shared_ptr<spdlog::logger> log;
  Logger() : log(spdlog::stdout_color_mt("is")) { log->set_pattern("[%l][%H:%M:%S:%e][%t] %v"); }
};

std::shared_ptr<spdlog::logger> logger() {
  static Logger logger;
  return logger.log;
}

}  // ::is

#endif  // __IS_LOGGER_HPP__