#ifndef __IS_DRIVER_PIONEER_HPP__
#define __IS_DRIVER_PIONEER_HPP__

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
#include "/usr/local/Aria/include/Aria.h"

namespace is {
namespace driver {

using namespace std::chrono;
using namespace is::msg::robot;
using namespace boost::lockfree;

template <typename T, size_t Size>
class SyncQueue {
 public:
  SyncQueue() { mtx.lock(); }

  bool push(const T& t) {
    bool ret = queue.push(t);
    mtx.unlock();
    return ret;
  }
  void wait() { mtx.lock(); }

  bool pop(T& t) { return queue.pop(t); }

 private:
  std::mutex mtx;
  boost::lockfree::spsc_queue<T, boost::lockfree::capacity<Size>> queue;
};

struct Pioneer {
  std::mutex mutex;
  std::thread robot_thread;
  ArRobot robot;
  ArSerialConnection serial_connection;
  ArTcpConnection tcp_connection;
  SyncQueue<Odometry, 10> odometry;

  Pioneer() {}

  Pioneer(std::string serial_port) {
    // std::lock_guard<std::mutex> lock(mutex);
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

  Pioneer(std::string hostname, int port) {
    // std::lock_guard<std::mutex> lock(mutex);
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

  Odometry get_odometry() {
    // std::lock_guard<std::mutex> lock(mutex);
    Odometry odo;
    odometry.wait();
    odometry.pop(odo);
    return odo;
  }

 private:
  void initialize() {
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
        // Get timestamp
        auto ts = system_clock::now();
        // Get odometry
        // mutex.lock();

        Odometry odo = {robot.getX(), robot.getY(), robot.getTh()};
        odometry.push(odo);
        // mutex.unlock();
        /*
        this->push_odometry(odo);
        // Sleep
        if (!(this->apply_delay.load())) {
          this_thread::sleep_until(ts + milliseconds(this->sample_rate));
        } else {
          this->apply_delay.store(false);
        */
        std::this_thread::sleep_until(ts + milliseconds(100));
      }
    });
    robot_thread = std::move(t);
  }
};

}  // ::driver
}  // ::is

#endif  // __IS_DRIVER_PIONEER_HPP__