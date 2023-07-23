#pragma once

#include "thread_pool.h"
#include <string>

namespace mltdl {

class DownloadManager {
public:
  DownloadManager(size_t max_concurrent_tasks = 10)
      : thread_pool_(max_concurrent_tasks) {}
  void addTask(const std::string &url, const std::string &filedir,
               bool large_file = false);
  void start(bool wait = true) { thread_pool_.executeAll(wait); };

private:
  void downloadFile(const std::string &url, const std::string filedir,
                    bool large_file = false);

  ThreadPool thread_pool_;
  std::mutex mutex_;
};
} // namespace mltdl
