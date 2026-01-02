#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <vector>

namespace quine {
namespace core {

/// @brief A simple thread-safe queue for inter-thread communication.
/// In a production shared-nothing system, this should be a lock-free SPSC or
/// MPSC queue. For V1, we use std::mutex + std::deque for correctness and
/// simplicity.
template <typename T> class ItcChannel {
public:
  ItcChannel() = default;

  /// @brief Push an item into the channel.
  /// Thread-safe (Multiple Producers).
  void push(T item) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(std::move(item));
    // TODO: Signal eventfd here to wake up the consumer's event loop!
  }

  /// @brief Try to pop an item from the channel.
  /// Thread-safe (Single Consumer mostly, but safe for multiple).
  /// @return std::nullopt if empty.
  std::optional<T> try_pop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      return std::nullopt;
    }
    T item = std::move(queue_.front());
    queue_.pop_front();
    return item;
  }

  /// @brief Consume all items currently in the queue.
  /// Useful for batch processing in the event loop.
  /// @param handler Function to call for each item.
  void consume_all(std::function<void(T &&)> handler) {
    std::deque<T> batch;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      batch.swap(queue_);
    }

    for (auto &item : batch) {
      handler(std::move(item));
    }
  }

  bool empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

private:
  std::deque<T> queue_;
  mutable std::mutex mutex_;
};

} // namespace core
} // namespace quine
