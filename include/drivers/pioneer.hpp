#ifndef __IS_DRIVER_PIONEER_HPP__
#define __IS_DRIVER_PIONEER_HPP__

#include <atomic>
#include <boost/lockfree/spsc_queue.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "../logger.hpp"
#include "../msgs/common.hpp"
#include "../msgs/robot.hpp"
#include <Aria/include/Aria.h>

namespace is {
namespace driver {

using namespace std::chrono;
using namespace is::msg::common;
using namespace is::msg::robot;
using namespace boost::lockfree;

template <typename T, size_t Size>
class sync_queue {
 public:
  sync_queue() { mtx.lock(); }

  bool push(const T& t) {
    bool ret = queue.push(t);
    mtx.unlock();
    return ret;
  }
  bool pop(T& t) {
    mtx.lock();
    return queue.pop(t);
  }

 private:
  std::mutex mtx;
  spsc_queue<T, capacity<Size>> queue;
};

struct Pioneer {
  std::mutex mutex;
  std::thread robot_thread;
  ArRobot robot;
  ArSerialConnection serial_connection;
  ArTcpConnection tcp_connection;
  sync_queue<Odometry, 10> odometry;
  int64_t period;  // [ms]
  int64_t delay;   // [ms]
  std::atomic<bool> apply_delay;
  TimeStamp last_timestamp;

  Pioneer() {}

  Pioneer(std::string serial_port) { connect(serial_port); }

  Pioneer(std::string hostname, int port) { connect(hostname, port); }

  void connect(std::string serial_port) {
    mutex.lock();
    Aria::init();
    if (serial_connection.open(serial_port.c_str()) != 0) {
      throw std::runtime_error("Cannot open port.");
    }
    robot.setDeviceConnection(&serial_connection);
    if (!robot.blockingConnect()) {
      throw std::runtime_error("Could not connect to robot.");
    }
    mutex.unlock();
    initialize();
  }

  void connect(std::string hostname, int port) {
    mutex.lock();
    Aria::init();
    if (tcp_connection.open(hostname.c_str(), port) != 0) {
      throw std::runtime_error("Cannot open port.");
    }
    robot.setDeviceConnection(&tcp_connection);
    if (!robot.blockingConnect()) {
      throw std::runtime_error("Could not connect to robot.");
    }
    mutex.unlock();
    initialize();
  }

  void set_velocities(Velocities vels) {
    std::lock_guard<std::mutex> lock(mutex);
    robot.lock();
    robot.setVel(vels.v);
    robot.setRotVel(vels.w);
    robot.unlock();
  }

  Velocities get_velocities() {
    std::lock_guard<std::mutex> lock(mutex);
    robot.lock();
    double linvel = robot.getVel();
    double rotvel = robot.getRotVel();
    robot.unlock();
    return {linvel, rotvel};
  }

  Odometry get_odometry() {
    Odometry odo;
    odometry.pop(odo);
    return odo;
  }

  void set_pose(Odometry pose) {
    std::lock_guard<std::mutex> lock(mutex);
    robot.lock();
    robot.moveTo(ArPose(pose.x, pose.y, pose.th));
    robot.unlock();
  }

  void set_sample_rate(SamplingRate sample_rate) {
    std::lock_guard<std::mutex> lock(mutex);
    if (sample_rate.rate == 0.0 && sample_rate.period > 0) {
      period = sample_rate.period < 100 ? 100 : sample_rate.period;
    } else if (sample_rate.rate > 0.0 && sample_rate.period == 0) {
      int64_t cur_period = static_cast<int64_t>(1000.0 / sample_rate.rate);
      period = cur_period < 100 ? 100 : cur_period;
    }
  }

  SamplingRate get_sample_rate() {
    std::lock_guard<std::mutex> lock(mutex);
    SamplingRate sample_rate;
    sample_rate.period = period;
    sample_rate.rate = static_cast<double>(1000 / period);
    return sample_rate;
  }

  void set_delay(Delay d) {
    std::lock_guard<std::mutex> lock(mutex);
    if (d.milliseconds <= period) {
      delay = d.milliseconds;
      apply_delay.store(true);
    }
  }

  TimeStamp get_last_timestamp() {
    std::lock_guard<std::mutex> lock(mutex);
    return last_timestamp;
  }

 private:
  void initialize() {
    period = 100;
    apply_delay.store(false);
    std::thread t([&]() {
      mutex.lock();
      robot.runAsync(true);
      robot.lock();
      robot.moveTo(ArPose(0.0, 0.0, 0.0));
      robot.enableMotors();
      robot.setVel(0.0);
      robot.setRotVel(0.0);
      robot.unlock();
      ArUtil::sleep(1000);
      mutex.unlock();

      while (1) {
        mutex.lock();
        // Get timestamp
        last_timestamp = TimeStamp();
        auto now = std::chrono::system_clock::now();
        // Get odometry
        Odometry odo = {robot.getX(), robot.getY(), robot.getTh()};
        mutex.unlock();
        odometry.push(odo);
        // Sleep
        if (!apply_delay.load()) {
          std::this_thread::sleep_until(now + milliseconds(period));
        } else {
          apply_delay.store(false);
          std::this_thread::sleep_until(now + milliseconds(period + delay));
        }
      }
    });
    robot_thread = std::move(t);
  }
};

}  // ::driver
}  // ::is

#endif  // __IS_DRIVER_PIONEER_HPP__