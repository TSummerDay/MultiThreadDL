#pragma once

#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace mltdl {

// as CLay is moving to fiber-based task system,
// the original thread-pool is still used for long-term tasks

class ThreadPool {
public:
  // Basic unit of work that our threads do
  typedef std::function<void(int32_t)> Work;

  ThreadPool(const char *name = "default");
  ThreadPool(int32_t num_thread, bool set_affinity = true,
             const char *name = "default");

  ~ThreadPool();

  // useful for post-construction init
  // e.g. the num_thread is available only after load cfg from file
  bool init(int32_t num_thread, bool set_affinity = true) noexcept;

  /**
   * @brief Adds work to the queue with optional priority.
   *        The work only gets queued and it will only start after invoking
   *        `execute_all` (wakes up all threads to complete all remaining works)
   * or `execute` (wakes up a single thread to complete one work unit).
   * @remarks if finished_adding_work == true, the thread pool will proceed
   * picking tasks from its queue, otherwise it will hold execution until
   * `execute_all` is invoked.
   */
  void enqueue(Work work, int64_t priority = 0,
               bool finished_adding_work = false) noexcept;

  /**
   * @brief Adds work to the queue with optional priority and wakes up a single
   *        thread that will pick the task in the queue with highest priority.
   */
  void spawn(Work work, int64_t priority = 0) noexcept;

  // helper function for async
  template <typename Fty, typename... Args,
            typename Rty = std::result_of_t<Fty()>>
  std::future<Rty> async(Fty &&func, Args &&...args) {
    auto bound_func =
        std::bind(std::forward<Fty>(func), std::forward<Args>(args)...);
    auto pro = std::make_shared<std::promise<Rty>>();
    auto future = pro->get_future();
    spawn([pro = std::move(pro), func = std::move(bound_func)](int) mutable {
      pro->set_value(func());
    });
    return future;
  }

  /**
   * @brief Wakes up all the threads to complete all the queued work,
   *        optionally not waiting for the work to be finished before return
   *        (the default wait=true is equivalent to invoking wait_for_completion
   * after execute_all).
   */
  void execute_all(bool wait = true);

  // shutdown all the threads in the pool
  void shutdown() noexcept;

  // abort all work
  void abort() noexcept;

  // return workqueue status
  bool queueempty() noexcept;

  /**
   * @brief Waits until all work issued to the thread pool is complete
   */
  void wait_for_completion(bool checkForErrors = true);

  // number of threads in the pool
  int size() const noexcept;

  // alias of number of threads in the pool
  int capacity() const noexcept { return size(); }

  std::string name() const noexcept { return name_; }

  std::vector<std::thread::id> thread_ids() const;

private:
  void thread_main(int thread_id, bool set_affinity);

  std::string name_;
  std::vector<std::thread> threads_;

  using PrioritizedWork = std::pair<int64_t, Work>;
  struct SortByPriority {
    bool operator()(const PrioritizedWork &a, const PrioritizedWork &b) {
      return a.first < b.first;
    }
  };
  std::priority_queue<PrioritizedWork, std::vector<PrioritizedWork>,
                      SortByPriority>
      work_queue_;

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
