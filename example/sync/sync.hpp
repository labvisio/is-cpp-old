#ifndef SYNC_SERVICE_UTILS_HPP
#define SYNC_SERVICE_UTILS_HPP

#include <is/is.hpp>
#include <is/msgs/common.hpp>

#include <boost/circular_buffer.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <tuple>

namespace is {

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace is::msg::common;

double mean(const boost::circular_buffer<double>& b) {
  return std::accumulate(b.begin(), b.end(), 0.0) / b.size();
}

void set_delays(is::ServiceClient& client, EntityList entities, vector<Delay> delays) {
  int it = 0;
  for (auto& e : entities.list) {
    client.request(e + ".set_delay", is::msgpack(delays[it++]));
  }
}

vector<vector<TimeStamp>> get_timestamps(is::Connection& is, vector<string> timestamp_tags, int n_samples) {
  vector<vector<TimeStamp>> timestamps;
  for (int n = 0; n < n_samples; ++n) {
    vector<TimeStamp> cur_ts;
    for (auto& t : timestamp_tags) {
      cur_ts.push_back(is::msgpack<TimeStamp>(is.consume(t)));
    }
    timestamps.push_back(cur_ts);
  }
  return timestamps;
}

vector<Delay> compute_delays(vector<vector<TimeStamp>> const& timestamps, SamplingRate sample_rate) {
  int ne = timestamps.front().size();
  vector<vector<int>> references(ne);
  vector<Delay> delays;

  for (auto& sample : timestamps) {
    static int cur_sample = 0;
    // Get the latest timestamp
    int64_t max = 0;
    int it_ref = 0;
    int i = 0;
    for (auto& ts : sample) {
      if (ts.time_point > max) {
        max = ts.time_point;
        it_ref = i;
      }
      i++;
    }
    references[it_ref].push_back(cur_sample++);
  }

  int max_size = 0;
  int it_max_size = 0;
  int it_size = 0;
  for (auto& r : references) {
    if ((int)r.size() > max_size) {
      max_size = r.size();
      it_max_size = it_size;
    }
    it_size++;
  }
  // Calculate delays between entities
  vector<boost::circular_buffer<double>> all_delays(ne);
  for (auto& d : all_delays) {
    d.set_capacity(references[it_max_size].size());
  }

  for (auto& r : references[it_max_size]) {
    int it = 0;
    for (auto& ts : timestamps[r]) {
      auto diff = nanoseconds(timestamps[r][it_max_size].time_point - ts.time_point);
      double dt = duration_cast<microseconds>(diff).count() / 1000.0;  // dt in milliseconds
      while (dt > 1000.0 / sample_rate.rate) {
        dt -= 1000.0 / sample_rate.rate;
      }
      if (dt > 950.0 / sample_rate.rate) {
        dt = 1000.0 / sample_rate.rate - dt;
      }
      all_delays[it++].push_back(dt);
    }
  }

  for (auto& d : all_delays) {
    for (auto& b : d) {
      cout << b << '\t';
    }
    cout << '\n';
  }

  for (auto& d : all_delays) {
    delays.push_back({static_cast<int>(std::round(mean(d)))});
    cout << delays.back().milliseconds << '\t' << mean(d) << endl;
  }

  return delays;
}

Status sync_entities(string uri, EntityList entities) {
  unsigned int ne = entities.list.size();
  vector<Delay> delays(ne, {0});
  auto is = is::connect(uri);
  auto client = is::make_client(is);

  // Verify entities sampling rate
  SamplingRate current_sr, previous_sr;
  bool first = true;
  for (auto& e : entities.list) {
    previous_sr = current_sr;
    auto req_id = client.request(e + ".get_sample_rate", is::msgpack(0));
    auto reply = client.receive(5s);
    if (reply == nullptr) {
      return Status::FAILED;
    } else if (reply->Message()->CorrelationId() != req_id) {
      is::logger()->error("Invalid reply");
    } else {
      current_sr = is::msgpack<SamplingRate>(reply);
      if (!first) {
        if (current_sr.rate != previous_sr.rate) {
          is::logger()->warn("Different sampling rate");
          return Status::FAILED;
        }
      } else {
        first = false;
      }
    }
  }

  // Create timestamp subscribers
  vector<string> timestamp_tags;
  for (auto& e : entities.list) {
    timestamp_tags.push_back(is.subscribe({e + ".timestamp"}));
  }

  // Set delays = zero
  set_delays(client, entities, delays);

  // Get 13 timestamps, discard 3
  auto samples = get_timestamps(is, timestamp_tags, 13);
  vector<vector<TimeStamp>> timestamps(samples.begin() + 3, samples.end());

  // Calculate delays
  delays = compute_delays(timestamps, current_sr);

  // Set delays
  set_delays(client, entities, delays);

  return Status::OK;
  ///////
  /*
  auto result = get_samples(subscribers, 10, different_fps);
  if (different_fps) {
    return Sync::FAILED;
  }
  auto ts_samples = result.first;
  delays = result.second;

  set_delays(clientDelay, entities, delays);

  // Verify sync

  // Discard 3 samples
  for (int i = 0; i < 3; i++) {
    for (auto& s : subscribers) {
      s.consume();
    }
  }

  result = get_samples(subscribers, 5, different_fps);
  ts_samples = result.first;
  delays = result.second;

  bool restart_sync = false;
  for (auto& d : delays) {
    if (d > 0.05 * (1000.0 / subscribers.front().fps)) {
      cout << "Restart sync" << endl;
      restart_sync = true;
    }
  }
  if (!restart_sync) {
    return Sync::OK;
  }
}
  */
}

}  // ::is

#endif  // SYNC_SERVICE_UTILS_HPP