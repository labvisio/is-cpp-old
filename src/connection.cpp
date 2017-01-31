#include "is/connection.hpp"

namespace is {

using namespace AmqpClient;
using namespace std::chrono;

Connection::Connection(std::string const& uri, std::string const& exchange) : Connection(make_channel(uri), exchange) {}

Connection::Connection(Channel::ptr_t channel, std::string const& exchange) : channel(channel), exchange(exchange) {
  // passive durable auto_delete
  channel->DeclareExchange(exchange, Channel::EXCHANGE_TYPE_TOPIC, false, false, false);
}

bool Connection::publish(std::string const& topic, BasicMessage::ptr_t message, bool mandatory) {
  try {
    if (!message->TimestampIsSet()) {
      set_timestamp(message);
    }
    channel->BasicPublish(exchange, topic, message, mandatory);
  } catch (MessageReturnedException) {
    return false;
  }
  return true;
}

std::string Connection::subscribe(std::string const& topic, int queue_size) {
  std::vector<std::string> topics{topic};
  return subscribe(topics, queue_size);
}

std::string Connection::subscribe(std::vector<std::string> const& topics, int queue_size) {
  // queue_name, passive, durable, exclusive, auto_delete
  std::string queue;
  if (queue_size) {
    Table arguments{{TableKey("x-max-length"), TableValue(queue_size)}};
    queue = channel->DeclareQueue("", false, false, true, true, arguments);
  } else {
    queue = channel->DeclareQueue("", false, false, true, true);
  }

  for (auto topic : topics) {
    channel->BindQueue(queue, exchange, topic);
  }

  // no_local, no_ack, exclusive, message_prefetch_count
  return channel->BasicConsume(queue, "", true, true, true);
}

Envelope::ptr_t Connection::consume(std::string const& tag) {
  return channel->BasicConsumeMessage(tag);
}

template <typename Time>
Envelope::ptr_t Connection::consume_for(std::string const& tag, Time const& timeout) {
  int timeout_ms = duration_cast<milliseconds>(timeout).count();
  Envelope::ptr_t envelope;
  channel->BasicConsumeMessage(tag, envelope, timeout_ms);
  return envelope;
}

std::vector<Envelope::ptr_t> Connection::consume_sync(std::vector<std::string> const& tags, int64_t period_ms) {
  std::vector<Envelope::ptr_t> envelopes;
  envelopes.reserve(tags.size());
  for (auto& tag : tags) {
    auto envelope = consume(tag);
    envelopes.emplace_back(envelope);
  }

  while (1) {
    auto minmax =
        std::minmax_element(std::begin(envelopes), std::end(envelopes), [](Envelope::ptr_t lhs, Envelope::ptr_t rhs) {
          return lhs->Message()->Timestamp() < rhs->Message()->Timestamp();
        });
    auto min = (*minmax.first)->Message()->Timestamp();
    auto max = (*minmax.second)->Message()->Timestamp();

    auto diff_ms = duration_cast<milliseconds>(nanoseconds(max - min)).count();
    if (diff_ms < period_ms) {
      break;
    }

    auto min_el = std::distance(std::begin(envelopes), minmax.first);
    envelopes[min_el] = consume(tags[min_el]);
  }

  return envelopes;
}

std::vector<Envelope::ptr_t> Connection::consume_sync(std::string const& tag, std::vector<std::string> topics,
                                                      int64_t period_ms) {
  std::vector<Envelope::ptr_t> envelopes(topics.size());
  std::sort(std::begin(topics), std::end(topics));

  int n_unitialized = topics.size();
  while (n_unitialized) {
    auto envelope = consume(tag);

    auto it = std::lower_bound(std::begin(topics), std::end(topics), envelope->RoutingKey());
    if (it != std::end(topics)) {
      auto el = std::distance(std::begin(topics), it);
      if (envelopes[el] == nullptr) {
        --n_unitialized;
      }
      envelopes[el] = envelope;
    }
  }

  while (1) {
    auto minmax =
        std::minmax_element(std::begin(envelopes), std::end(envelopes), [](Envelope::ptr_t lhs, Envelope::ptr_t rhs) {
          return lhs->Message()->Timestamp() < rhs->Message()->Timestamp();
        });
    auto min = (*minmax.first)->Message()->Timestamp();
    auto max = (*minmax.second)->Message()->Timestamp();

    auto diff_ms = duration_cast<milliseconds>(nanoseconds(max - min)).count();
    if (diff_ms < period_ms) {
      break;
    }

    auto envelope = consume(tag);
    auto it = std::lower_bound(std::begin(topics), std::end(topics), envelope->RoutingKey());
    if (it != std::end(topics)) {
      auto el = std::distance(std::begin(topics), it);
      envelopes[el] = envelope;
    }
  }

  return envelopes;
}

}  // ::is