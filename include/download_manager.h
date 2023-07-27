#pragma once

#include "curl_pool.h"
#include "thread_pool.h"
#include <curl/curl.h>
#include <queue>
#include <string>

namespace mltdl {

class DownloadManager {
public:
  DownloadManager(size_t max_concurrent_tasks = 10)
      : thread_pool_(max_concurrent_tasks), curl_pool_(max_concurrent_tasks) {}
  void addTask(const std::string &url, const std::string &filedir,
               bool large_file = false);
  void start(bool wait = true) { thread_pool_.executeAll(wait); };

private:
  void downloadFile(const std::string &url, const std::string filedir,
                    bool large_file = false);

  ThreadPool thread_pool_;
  CurlPool curl_pool_;
  std::mutex mutex_;
};
} // namespace mltdl
