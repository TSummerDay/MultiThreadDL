#include "thread_pool.h"

#include <iostream>

namespace mltdl {

ThreadPool::ThreadPool(const char *name /*=""*/)
    : name_(name), running_(true), work_complete_(true), adding_work_(false),
      active_threads_(0) {}

ThreadPool::ThreadPool(int num_thread, const char *name /*=""*/)
    : ThreadPool(name) {
  init(num_thread);
}

ThreadPool::~ThreadPool() { shutdown(); }

bool ThreadPool::init(int32_t num_thread) {
  num_thread =
      num_thread == -1 ? (int)std::thread::hardware_concurrency() : num_thread;
  threads_.resize(num_thread);
  // Start the threads in the main loop
  for (int i = 0; i < num_thread; ++i) {
    threads_[i] = std::thread(std::bind(&ThreadPool::threadMain, this, i));
  }
  tl_errors_.resize(num_thread);
  return true;
}

void ThreadPool::enqueue(Work work, bool finished_adding_work) {
  std::lock_guard<std::mutex> lock(mutex_);
  work_queue_.push(std::move(work));
  work_complete_ = false;
  adding_work_ = !finished_adding_work;
}

void ThreadPool::spawn(Work work) {
  enqueue(std::move(work), true);
  // Signal a thread to complete the work
  condition_.notify_one();
}

// abort all work and shutdown all threads
void ThreadPool::abort() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (!work_queue_.empty())
    work_queue_.pop();
}

bool ThreadPool::queueempty() { return work_queue_.empty(); }

// shutdown all the threads in the pool
void ThreadPool::shutdown() {
  waitForCompletion(false);
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
void ThreadPool::waitForCompletion(bool checkForErrors) {
  std::unique_lock<std::mutex> lock(mutex_);
  completed_.wait(lock, [this] { return this->work_complete_; });

  if (checkForErrors) {
    // Check for errors
    for (size_t i = 0; i < threads_.size(); ++i) {
      while (!tl_errors_[i].empty()) {
        // print the error that occurred
        std::cerr << "Thread id :" << i << " error : " << tl_errors_[i].front()
                  << std::endl;
        tl_errors_[i].pop();
      }
    }
  }
}

// Wake up an equal number of threads to execute the task, but not more than the
// maximum number of threads
void ThreadPool::executeAll(bool wait) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    adding_work_ = false;
  }
  condition_.notify_one();
  if (wait) {
    waitForCompletion();
  }
}

int ThreadPool::size() const { return threads_.size(); }

std::vector<std::thread::id> ThreadPool::thread_ids() const {
  std::vector<std::thread::id> tids;
  tids.reserve(threads_.size());
  for (const auto &thread : threads_)
    tids.emplace_back(thread.get_id());
  return tids;
}

void ThreadPool::threadMain(int thread_id) {
  // DeviceGuard g(device_id);
  while (running_) {
    // Block on the condition to wait for work
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this] {
      return !running_ || (!work_queue_.empty() && !adding_work_);
    });

    // If we're no longer running, exit the run loop
    if (!running_) {
      break;
    }

    // Get work from the queue & mark
    // this thread as active
    Work work = std::move(work_queue_.front());
    work_queue_.pop();
    bool should_wake_next = !work_queue_.empty();
    ++active_threads_;

    // Unlock the lock
    lock.unlock();

    if (should_wake_next) {
      condition_.notify_one();
    }

    // If an error occurs, we save it in tl_errors_. When
    // waitForCompletion is called, we will check for any errors
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
