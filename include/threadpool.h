#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace mltdl {
class ThreadPool {
public:
  ThreadPool(size_t num_threads) {
    stop_ = false;
    threads_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
      threads_.emplace_back([this]() {
        while (true) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(this->mutex_);
            this->condition_.wait(lock, [this]() {
              return this->stop_ || !this->tasks_.empty();
            });
            if (this->stop_ && this->tasks_.empty()) {
              return;
            }
            task = std::move(this->tasks_.front());
            this->tasks_.pop();

            bool should_wake_next = !this->tasks_.empty();

            lock.unlock();

            if (should_wake_next) {
              this->condition_.notify_one();
            }
          }
          task();
        }
      });
    }
  }

  template <class F> void enqueue(F &&f) {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.emplace(std::forward<F>(f));
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      stop_ = true;
    }
    condition_.notify_all();
    for (auto &worker : threads_) {
      worker.join();
    }
  }

private:
  std::vector<std::thread> threads_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  std::condition_variable condition_;
  std::atomic<bool> stop_;
};

} // namespace mltdl