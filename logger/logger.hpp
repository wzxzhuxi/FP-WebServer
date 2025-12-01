#pragma once
#include "sink.hpp"
#include "writer.hpp"
#include <vector>
#include <algorithm>
#include <functional>

namespace logger {

using Filter = std::function<bool(const LogEntry&)>;

inline Filter level_filter(Level min_level) {
  return [min_level](const LogEntry& entry) {
    return entry.level >= min_level;
  };
}

class Logger {
  std::vector<std::shared_ptr<Sink>> sinks_;
  Level min_level_;
  Filter filter_;

public:
  explicit Logger(Level min_level = Level::Info) : min_level_(min_level), filter_(level_filter(min_level)) {}

  Logger with_sink(std::shared_ptr<Sink> sink) const {
    Logger new_logger = *this;
    new_logger.sinks_.push_back(std::move(sink));
    return new_logger;
  }

  Logger with_filter(Filter filter) const {
    Logger new_logger = *this;
    new_logger.filter_ = std::move(filter);
    return new_logger;
  }

  void log(LogEntry entry) const {
    if (!filter_(entry)) {
      return;
    }

    for (const auto& sink : sinks_) {
      sink->write(entry);
    }
  }

  void info(std::string message, std::string file = "", int line = 0) const {
    log(LogEntry{
      .level = Level::Info,
      .message = std::move(message),
      .timestamp = std::chrono::system_clock::now(),
      .file = std::move(file),
      .line = line,
      .function = {},
      .fields = {}
    });
  }

  void error(std::string message, std::string file = "", int line = 0) const {
    log(LogEntry{
      .level = Level::Error,
      .message = std::move(message),
      .timestamp = std::chrono::system_clock::now(),
      .file = std::move(file),
      .line = line,
      .function = {},
      .fields = {}
    });
  }

  template<typename T>
  void write_logged(const Logged<T>& logged) const {
    for (const auto& entry : logged.logs()) {
      log(entry);
    }
  }

  void flush() const {
    for (const auto& sink : sinks_) {
      sink->flush();
    }
  }
};

}

#define LOG_INFO(logger, msg) \
(logger).info(msg, __FILE__, __LINE__);
#define LOG_ERROR(logger, msg) \
(logger).error(msg, __FILE__, __LINE__);
