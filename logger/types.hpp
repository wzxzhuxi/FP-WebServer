#pragma once
#include <string>
#include <chrono>
#include <unordered_map>
#include <sstream>
#include <format>
#include <iomanip>

namespace logger {

enum class Level {
  Trace,
  Debug,
  Info,
  Warn,
  Error,
  Fatal
};

inline std::string_view level_to_string(Level level) {
  switch (level) {
    case Level::Trace:
      return "TRACE";
    case Level::Debug:
      return "DEBUG";
    case Level::Info:
      return "INFO";
    case Level::Warn:
      return "WARN";
    case Level::Error:
      return "ERROR";
    case Level::Fatal:
      return "FATAL";
  }
  return "UNKNOWN";
}

struct LogEntry {
  Level level;
std::string message;
std::chrono::system_clock::time_point timestamp;
  std::string file;
  int line;
  std::string function;
  std::unordered_map<std::string, std::string> fields;

  std::string format() const {
    std::ostringstream oss;

    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

    oss << " [" << level_to_string(level) << "]";

    oss << " " << file << ":" <<line;

    oss << " " << message;

    if (!fields.empty()) {
      oss << " {";
      bool first = true;
      for (const auto& [k, v] : fields) {
        if (!first) oss << ", ";
        oss << k << "=" << v;
        first = false;
      }
      oss << "}";
    }
    oss << "\n";
    return oss.str();
  }
};

}
