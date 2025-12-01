#pragma once
#include "types.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <iomanip>
#include <thread>
#include "threadpool/channel.hpp"

namespace logger{

class Sink {
public:
virtual ~Sink() = default;
virtual void write(const logger::LogEntry &entry) = 0;
virtual void flush() = 0;
}; // namespace Sink

class ConsoleSink : public Sink {
  mutable std::mutex mutex_;

public:
  void write(const logger::LogEntry &entry) override {
    std::lock_guard lock(mutex_);
    std::cout << entry.format() << std::flush;
  }

  void flush() override { std::cout << std::flush; }
};

class FileSink : public Sink {
  std::ofstream file_;
  mutable std::mutex mutex_;
  std::string filepath_;

public:
  explicit FileSink(std::string filepath) : filepath_(std::move(filepath)) {
    file_.open(filepath_, std::ios::app);
  }
  void write(const logger::LogEntry &entry) override {
    std::lock_guard lock(mutex_);
    if (file_.is_open()) {
      file_ << entry.format();
    }
  }
  void flush() override {
    std::lock_guard lock(mutex_);
    if (file_.is_open()) {
      file_.flush();
    }
  }
  ~FileSink() override {
    if (file_.is_open()) {
      file_.close();
    }
  }
};

class RotatingFileSink : public Sink {
  std::string base_dir_;
  std::string base_name_;
  size_t max_lines_;
  size_t total_count_ = 0;
  std::ofstream file_;
  mutable std::mutex mutex_;

  int current_day_ = 0;
  static int get_today() {
    time_t t = time(nullptr);
    struct tm *sys_tm = localtime(&t);
    return sys_tm->tm_mday;
  }

  std::string generate_filepath(bool is_line_rotation) const {
    time_t t = time(nullptr);
    struct tm *sys_tm = localtime(&t);

    std::ostringstream oss;
    oss << base_dir_ << "/" << (sys_tm->tm_year + 1900) << "_"
        << std::setfill('0') << std::setw(2) << (sys_tm->tm_mon + 1) << "_"
        << std::setfill('0') << std::setw(2) << sys_tm->tm_mday << "_"
        << std::setfill('0') << std::setw(2) << sys_tm->tm_hour << "_"
        << std::setfill('0') << std::setw(2) << sys_tm->tm_min << "_"
        << std::setfill('0') << std::setw(2) << sys_tm->tm_sec << "_"
        << base_name_ << ".log";

    if (is_line_rotation) {
      oss << "." << (total_count_ / max_lines_);
    }

    return oss.str();
  }
  bool should_rotate(int today) const {
    return (current_day_ != today) || (total_count_ % max_lines_ == 0);
  }
  void rotate(int today) {
    if (file_.is_open()) {
      file_.flush();
      file_.close();
    }

    bool is_line_rotation = (current_day_ == today);

    if (current_day_ != today) {
      current_day_ = today;
      total_count_ = 0;
    }

    std::string new_path = generate_filepath(is_line_rotation);
    file_.open(new_path, std::ios::app);

    if (!file_.is_open()) {
      std::cerr << "Failed to open rotate log file: " << new_path << std::endl;
    }
  }

public:
  explicit RotatingFileSink(std::string base_dir, std::string base_name,
                            size_t max_lines = 5000000)
      : base_dir_(std::move(base_dir)), base_name_(std::move(base_name)),
        max_lines_(max_lines), current_day_(get_today()) {
    rotate(current_day_);
  }

  void write(const LogEntry &entry) override {
    std::lock_guard lock(mutex_);
    total_count_++;
    int today = get_today();
    if (should_rotate(today)) {
      rotate(today);
    }

    if (file_.is_open()) {
      file_ << entry.format();
    }
  }

  void flush() override {
    std::lock_guard lock(mutex_);
    if (file_.is_open()) {
      file_.flush();
    }
  }

  ~RotatingFileSink() override {
    if (file_.is_open()) {
      file_.close();
    }
  }
};

class AsyncSink : public Sink {
  std::unique_ptr<Sink> inner_sink_;
  std::shared_ptr<threadpool::Channel<LogEntry>> channel_;
  std::jthread worker_;

public:
  explicit AsyncSink(std::unique_ptr<Sink> sink, size_t buffer_size = 1000)
      : inner_sink_(std::move(sink)),
        channel_(std::make_shared<threadpool::Channel<LogEntry>>(buffer_size)),
        worker_([this](std::stop_token stoken) { worker_loop(stoken); }) {}

  void write(const LogEntry& entry) override {
    channel_->try_send(entry);
  }
  void flush() override {
    while (channel_->size() > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    inner_sink_ -> flush();
  }

  ~AsyncSink() override {
    channel_->close();
  }

private:
  void worker_loop(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
      auto entry = channel_->recv();
      if (!entry) break;
      inner_sink_->write(*entry);
    }
  }
};

}
