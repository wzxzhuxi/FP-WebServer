#pragma once
#include "types.hpp"
#include <functional>
#include <vector>

namespace logger {

template <typename T> class Logged {
  T value_;
  std::vector<LogEntry> logs_;

public:
  Logged(T value, std::vector<LogEntry> logs = {})
      : value_(std::move(value)), logs_(std::move(logs)) {}

  const T &value() const { return value_; }
  const std::vector<LogEntry> &logs() const { return logs_; }

  template <typename F> auto map(F &&f) const {
    using U = decltype(f(value_));
    return Logged<U>(f(value_), logs_);
  }

  template <typename F> auto flat_map(F &&f) const {
    auto result = f(value_);
    auto combined_logs = logs_;
    combined_logs.insert(combined_logs.end(), result.logs_.begin(),
                         result.logs_.end());
    using U = decltype(result.value_);
    return Logged<U>(std::move(result.value_), std::move(combined_logs));
  }

  Logged<T> with_log(LogEntry entry) const {
    auto new_logs = logs_;
    new_logs.push_back(std::move(entry));
    return Logged<T>(value_, std::move(new_logs));
  }

  Logged<T> log_info(std::string message) const {
    return with_log(LogEntry{.level = Level::Info,
                             .message = std::move(message),
                             .timestamp = std::chrono::system_clock::now(),
                             .file = {},
                             .line = 0,
                             .function = {},
                             .fields = {}});
  }

  Logged<T> log_error(std::string message) const {
    return with_log(LogEntry{.level = Level::Error,
                             .message = std::move(message),
                             .timestamp = std::chrono::system_clock::now(),
                             .file = {},
                             .line = 0,
                             .function = {},
                             .fields = {}});
  }
};

template <typename T> Logged<T> pure(T value) {
  return Logged<T>(std::move(value));
}

} // namespace logger
