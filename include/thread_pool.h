#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace mltdl {

class ThreadPool {
public:
  // Basic unit of work that our threads do
  using Work = std::function<void(int32_t)>;

  ThreadPool(const char *name = "default");
  ThreadPool(int32_t num_thread, const char *name = "default");

  ~ThreadPool();

  // useful for post-construction init
  bool init(int32_t num_thread);

  /**
   * Add a work to the queue
   * if finished_adding_work == true, the thread pool will proceed
   * picking tasks from its queue, otherwise it will hold execution until
   * `executeAll` is invoked.
   */
  void enqueue(Work work, bool finished_adding_work = false);

  // Add a work to the queue and wakes up a thread to do it
  void spawn(Work work);

  /**
   * Wakes up all the threads to complete all the queued work,
   * optionally not waiting for the work to be finished before return
   * (the default wait=true is equivalent to invoking wait_for_completion
   * after executeAll).
   */
  void executeAll(bool wait = true);

  // shutdown all the threads in the pool
  void shutdown();

  // abort all work
  void abort();

  // return workqueue status
  bool queueempty();

  //  Waits until all work issued to the thread pool is complete
  void waitForCompletion(bool checkForErrors = true);

  // number of threads in the pool
  int size() const;

  // alias of number of threads in the pool
  int capacity() const { return size(); }

  std::string name() const { return name_; }

  std::vector<std::thread::id> thread_ids() const;

private:
  void threadMain(int thread_id);

  std::string name_;
  std::vector<std::thread> threads_;
  std::queue<Work> work_queue_;

  bool running_;
  bool work_complete_;
  bool adding_work_;
  int32_t active_threads_;
  std::mutex mutex_;
  std::condition_variable condition_;
  std::condition_variable completed_;

  //  Stored error strings for each thread
  std::vector<std::queue<std::string>> tl_errors_;
};

} // namespace mltdl
