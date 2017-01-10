#ifndef __IS_TIMER_HPP__
#define __IS_TIMER_HPP__

#include <mutex>
#include <chrono>
#include <ev.h>
#include <boost/fiber/all.hpp>

namespace is {

struct Timer {
  ev_timer timer;

  boost::fibers::condition_variable condition;
  boost::fibers::mutex mutex;
  bool run = false;

  template <typename Time, typename Fn, typename... Args>
  Timer(Time&& time, Fn fn, Args&&... args) {
    boost::fibers::fiber fiber(
        [this](Fn&& fn, Args&&... args) {
          while (1) {
            std::unique_lock<boost::fibers::mutex> lk(mutex);
            while (!run) {
              condition.wait(lk);
            }
            fn(std::forward<Args>(args)...);
            run = false;
          }
        },
        std::forward<Fn>(fn), std::forward<Args>(args)...);

    fiber.detach();

    auto callback = [](struct ev_loop* loop, ev_timer* timer, int revents) {
      Timer* thys = (Timer*)timer->data;
      {
        std::unique_lock<boost::fibers::mutex> lk(thys->mutex);
        thys->run = true;
      }
      thys->condition.notify_all();
      boost::this_fiber::yield();
    };

    timer.data = this;

    double timeout =
        std::chrono::duration_cast<std::chrono::nanoseconds>(time).count() /
        1.0e9;
    ev_timer_init(&timer, callback, timeout, timeout);
    ev_timer_start(EV_DEFAULT, &timer);
  }

  Timer(Timer const&) = delete;
  Timer& operator=(Timer const&) = delete;
};  // Timer

}  // ::is

#endif  // __IS_TIMER_HPP__