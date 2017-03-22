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

namespace log {

template <class... Args>
void info(Args&&... args) {
  logger()->info(args...);
}

template <class... Args>
void warn(Args&&... args) {
  logger()->warn(args...);
}

template <class... Args>
void error(Args&&... args) {
  logger()->error(args...);
}

template <class... Args>
void critical(Args&&... args) {
  logger()->critical(args...);
  exit(-1);
}

}

}  // ::is

#endif  // __IS_LOGGER_HPP__