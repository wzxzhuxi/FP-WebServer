#pragma once
#include "channel.hpp"
#include "task.hpp"
#include <memory>
#include <thread>
#include <vector>
#include <atomic>

namespace threadpool {

// 检测类型是否已经是 TaskResult<T>
template <typename T>
struct is_task_result : std::false_type {};

template <typename T>
struct is_task_result<TaskResult<T>> : std::true_type {
  using value_type = T;
};

template <typename T>
inline constexpr bool is_task_result_v = is_task_result<T>::value;

template <typename T>
struct unwrap_task_result {
  using type = T;
};

template <typename T>
struct unwrap_task_result<TaskResult<T>> {
  using type = T;
};

template <typename T>
using unwrap_task_result_t = typename unwrap_task_result<T>::type;

class ThreadPool {
  std::vector<std::jthread> workers_;
  Channel<AnyTask> task_channel_;
  std::atomic<bool> running_{true};

public:
  explicit ThreadPool(size_t num_threads, size_t max_queue_size = 10000)
      : task_channel_(max_queue_size) {

    workers_.reserve(num_threads);

    for (size_t i = 0; i < num_threads; ++i) {
      workers_.emplace_back(
          [this](std::stop_token stoken) { worker_loop(stoken); });
    }
  }

  ~ThreadPool() { shutdown(); }

  template <typename F, typename... Args>
  [[nodiscard]] auto submit(F &&f, Args &&...args)
      -> std::future<TaskResult<unwrap_task_result_t<std::invoke_result_t<F, Args...>>>> {
    using RawReturnType = std::invoke_result_t<F, Args...>;
    using UnwrappedType = unwrap_task_result_t<RawReturnType>;

    Task<UnwrappedType> bound_task = [f = std::forward<F>(f),
                       ... args = std::forward<Args>(
                           args)]() mutable -> TaskResult<UnwrappedType> {
      try {
        if constexpr (std::is_void_v<RawReturnType>) {
          std::invoke(f, std::move(args)...);
          return {};
        } else if constexpr (is_task_result_v<RawReturnType>) {
          // F 已经返回 TaskResult<T>，直接返回
          return std::invoke(f, std::move(args)...);
        } else {
          // F 返回普通类型 T，包装为 TaskResult<T>
          return std::invoke(f, std::move(args)...);
        }
      } catch (const std::exception &e) {
        return std::unexpected(e.what());
      } catch (...) {
        return std::unexpected("Unknown error");
      }
    };

    std::promise<TaskResult<UnwrappedType>> promise;
    auto future = promise.get_future();

    auto any_task = AnyTask::create(std::move(bound_task), std::move(promise));

    if (!task_channel_.send(std::move(any_task))) {
      std::promise<TaskResult<UnwrappedType>> error_promise;
      error_promise.set_value(std::unexpected("Thread pool shut down"));
      return error_promise.get_future();
    }

    return future;
  }

  void shutdown() {
    if (running_.exchange(false)) {
      task_channel_.close();

      for (auto &worker : workers_) {
        worker.request_stop();
      }
    }
  }

  size_t pending_tasks() const { return task_channel_.size(); }

private:
  void worker_loop(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
      auto task = task_channel_.recv();

      if (!task) {
        break;
      }

      task->execute();
    }
  }
};
} // namespace threadpool
