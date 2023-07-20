#include "thread_pool.h"

#include <cstdlib>
#include <utility>

namespace mltdl {

ThreadPool::ThreadPool(const char *name /*=""*/)
    : name_(name), running_(true), work_complete_(true), adding_work_(false),
      active_threads_(0) {}

ThreadPool::ThreadPool(int num_thread, bool set_affinity,
                       const char *name /*=""*/)
    : ThreadPool(name) {
  init(num_thread, set_affinity);
}

ThreadPool::~ThreadPool() { shutdown(); }

bool ThreadPool::init(int32_t num_thread,
                      bool set_affinity /*= true*/) noexcept {
  num_thread =
      num_thread == -1 ? (int)std::thread::hardware_concurrency() : num_thread;
  threads_.resize(num_thread);
  // Start the threads in the main loop
  for (int i = 0; i < num_thread; ++i) {
    threads_[i] =
        std::thread(std::bind(&ThreadPool::thread_main, this, i, set_affinity));
  }
  tl_errors_.resize(num_thread);
  return true;
}

void ThreadPool::enqueue(Work work, int64_t priority,
                         bool finished_adding_work) noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  work_queue_.push({priority, std::move(work)});
  work_complete_ = false;
  adding_work_ = !finished_adding_work;
}

void ThreadPool::spawn(Work work, int64_t priority) noexcept {
  enqueue(std::move(work), priority, true);
  // Signal a thread to complete the work
  condition_.notify_one();
}

// abort all work and shutdown all threads
void ThreadPool::abort() noexcept {
  std::unique_lock<std::mutex> lock(mutex_);
  while (!work_queue_.empty())
    work_queue_.pop();
}

bool ThreadPool::queueempty() noexcept { return work_queue_.empty(); }

// shutdown all the threads in the pool
void ThreadPool::shutdown() noexcept {
  wait_for_completion(false);
  std::unique_lock<std::mutex> lock(mutex_);
  running_ = false;
  condition_.notify_all();
  lock.unlock();

  for (auto &thread : threads_) {
    thread.join();
  }
  threads_.clear();
}

// Blocks until all work issued to the thread pool is complete
void ThreadPool::wait_for_completion(bool checkForErrors) {
  std::unique_lock<std::mutex> lock(mutex_);
  completed_.wait(lock, [this] { return this->work_complete_; });

  if (checkForErrors) {
    // Check for errors
    for (size_t i = 0; i < threads_.size(); ++i) {
      if (!tl_errors_[i].empty()) {
        // Throw the first error that occurred
        auto error = "Error in thread";
        tl_errors_[i].pop();
        throw std::runtime_error(error);
      }
    }
  }
}

void ThreadPool::execute_all(bool wait) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    adding_work_ = false;
  }
  condition_.notify_one();
  if (wait) {
    wait_for_completion();
  }
}

int ThreadPool::size() const noexcept { return threads_.size(); }

std::vector<std::thread::id> ThreadPool::thread_ids() const {
  std::vector<std::thread::id> tids;
  tids.reserve(threads_.size());
  for (const auto &thread : threads_)
    tids.emplace_back(thread.get_id());
  return tids;
}

void ThreadPool::thread_main(int thread_id, bool set_affinity) {
  // DeviceGuard g(device_id);
  while (running_) {
    // Block on the condition to wait for work
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this] {
      return !running_ || (!work_queue_.empty() && !adding_work_);
    });

    // If we're no longer running, exit the run loop
    if (!running_)
      break;

    // Get work from the queue & mark
    // this thread as active
    Work work = std::move(work_queue_.top().second);
    work_queue_.pop();
    bool should_wake_next = !work_queue_.empty();
    ++active_threads_;

    // Unlock the lock
    lock.unlock();

    if (should_wake_next) {
      condition_.notify_one();
    }

    // If an error occurs, we save it in tl_errors_. When
    // wait_for_completion is called, we will check for any errors
    // in the threads and return an error if one occurs.
    try {
      work(thread_id);
    } catch (std::exception &e) {
      lock.lock();
      tl_errors_[thread_id].push(e.what());
      lock.unlock();
    } catch (...) {
      lock.lock();
      tl_errors_[thread_id].push("Caught unknown exception");
      lock.unlock();
    }

    // Mark this thread as idle & check for complete work
    lock.lock();
    --active_threads_;
    if (work_queue_.empty() && active_threads_ == 0) {
      work_complete_ = true;
      completed_.notify_one();
    }
  }
}

} // namespace mltdl
