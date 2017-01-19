#ifndef __IS_DRIVER_PIONEER_HPP__
#define __IS_DRIVER_PIONEER_HPP__

#include <is/msgs/common.hpp>
#include <is/msgs/robot.hpp>
#include <is/logger.hpp>

#include <chrono>
#include <memory>
#include <string>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <Aria/include/Aria.h>

namespace is {
namespace driver {

using namespace std::chrono;
using namespace is::msg::common;
using namespace is::msg::robot;

struct Pioneer {
  ArRobot robot;
  ArDeviceConnection* connection;

  TimeStamp last_timestamp;
  std::atomic<int64_t> period_ms;
  std::atomic<int64_t> delay_ms;

  bool ready;
  std::mutex mutex;
  std::condition_variable condition;
  std::thread thread;
  std::atomic<bool> running;

  Pioneer() : connection(nullptr) {}
  Pioneer(std::string const& port) { connect(port); }
  Pioneer(std::string const& hostname, int port) { connect(hostname, port); }

  virtual ~Pioneer() {
    running = false;
    thread.join();
    if (connection != nullptr) {
      delete connection;
    }
  }

  void connect(std::string const& port) {
    Aria::init();
    auto serial = new ArSerialConnection;
    connection = serial;
    serial->open(port.c_str());
    robot.setDeviceConnection(serial);
    initialize();
  }

  void connect(std::string const& hostname, int port) {
    Aria::init();
    auto tcp = new ArTcpConnection;
    connection = tcp;
    tcp->open(hostname.c_str(), port);
    robot.setDeviceConnection(tcp);
    initialize();
  }

  void set_velocities(Velocities const& vels) {
    robot.lock();
    robot.setVel(vels.v);
    robot.setRotVel(vels.w);
    robot.unlock();
  }

  Velocities get_velocities() {
    robot.lock();
    double linear = robot.getVel();
    double angular = robot.getRotVel();
    robot.unlock();
    return {linear, angular};
  }

  Odometry get_odometry() {
    robot.lock();
    ArPose pose = robot.getPose();
    robot.unlock();
    Odometry odometry{pose.getX(), pose.getY(), pose.getTh()};
    return odometry;
  }

  void set_pose(Odometry const& pose) {
    robot.lock();
    robot.moveTo(ArPose(pose.x, pose.y, pose.th));
    robot.unlock();
  }

  void set_sample_rate(SamplingRate const& rate) {
    if (rate.period != 0) {
      period_ms = std::max<int64_t>(100, rate.period);
    } else if (rate.rate != 0.0) {
      period_ms = std::max<int64_t>(100, 1000.0 / rate.rate);
    }
  }

  SamplingRate get_sample_rate() {
    SamplingRate rate;
    rate.period = period_ms;
    rate.rate = static_cast<double>(1000.0 / period_ms);
    return rate;
  }

  void set_delay(Delay const& delay) {
    if (delay.milliseconds <= period_ms) {
      delay_ms = delay.milliseconds;
    }
  }

  TimeStamp get_last_timestamp() { return last_timestamp; }

  void wait() {
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [this] { return ready; });
    ready = false;
  }

 private:
  void initialize() {
    if (!robot.blockingConnect()) {
      throw std::runtime_error("Could not connect to robot.");
    }

    robot.runAsync(true);
    robot.lock();
    robot.moveTo(ArPose(0.0, 0.0, 0.0));
    robot.enableMotors();
    robot.setVel(0.0);
    robot.setRotVel(0.0);
    robot.unlock();

    period_ms = 100;
    delay_ms = 0;

    auto loop = [this]() {
      running = true;
      while (running) {
        auto now = std::chrono::system_clock::now();
        last_timestamp = TimeStamp();

        std::this_thread::sleep_until(now + milliseconds(period_ms + delay_ms));
        
        {
          std::unique_lock<std::mutex> lock(mutex);
          ready = true;
        }
        condition.notify_one();
      }
    };

    thread = std::thread(loop);
  }
};

}  // ::driver
}  // ::is

#endif  // __IS_DRIVER_PIONEER_HPP__