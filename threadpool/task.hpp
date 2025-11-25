#pragma once
#include <expected>
#include <functional>
#include <future>
#include <memory>

namespace threadpool {

template <typename T> using TaskResult = std::expected<T, std::string>;

template <typename T> using Task = std::function<TaskResult<T>()>;

class AnyTask {
  struct TaskBase {
    virtual ~TaskBase() = default;
    virtual void execute() = 0;
  };
  template <typename T> struct TaskImpl : TaskBase {
    Task<T> task;
    std::promise<TaskResult<T>> promise;

    explicit TaskImpl(Task<T> t) : task(std::move(t)) {}

    void execute() override {
      try {
        auto result = task();
        promise.set_value(std::move(result));
      } catch (...) {
        promise.set_exception(std::current_exception());
      }
    }
  };
  std::unique_ptr<TaskBase> task_;

public:
  template <typename T>
  static AnyTask create(Task<T> task, std::promise<TaskResult<T>> promise) {
    AnyTask any_task;
    auto impl = std::make_unique<TaskImpl<T>>(std::move(task));
    impl->promise = std::move(promise);
    any_task.task_ = std::move(impl);
    return any_task;
  }
  void execute() {
    if (task_) {
      task_->execute();
    }
  }
};
} // namespace threadpool
