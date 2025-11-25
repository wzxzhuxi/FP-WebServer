#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace threadpool {

template <typename T> class Channel {
  std::queue<T> queue_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  bool closed_ = false;
  size_t max_size_;

public:
  explicit Channel(size_t max_size = SIZE_MAX) : max_size_(max_size) {}

  bool send(T value) {
    std::unique_lock lock(mutex_);

    cv_.wait(lock, [this] { return queue_.size() < max_size_ || closed_; });

    if (closed_) {
      return false;
    }

    queue_.push(std::move(value));
    cv_.notify_one();
    return true;
  }

  bool try_send(T value) {
    std::unique_lock lock(mutex_);

    if (closed_ || queue_.size() >= max_size_) {
      return false;
    }

    queue_.push(std::move(value));
    cv_.notify_one();
    return true;
  }

  std::optional<T> recv() {
    std::unique_lock lock(mutex_);

    cv_.wait(lock, [this] { return !queue_.empty() || closed_; });

    if (queue_.empty()) {
      return std::nullopt;
    }

    T value = std::move(queue_.front());
    queue_.pop();
    cv_.notify_one();
    return value;
  }

  std::optional<T> try_recv() {
    std::unique_lock lock(mutex_);

    if (queue_.empty()) {
      return std::nullopt;
    }

    T value = std::move(queue_.front());
    queue_.pop();
    cv_.notify_one();
    return value;
  }

  void close() {
    std::unique_lock lock(mutex_);
    closed_ = true;
    cv_.notify_all();
  }

  size_t size() const {
    std::unique_lock lock(mutex_);
    return queue_.size();
  }
};
} // namespace threadpool
