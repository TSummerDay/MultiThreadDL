#pragma once

#include "curl_pool.h"
#include "thread_pool.h"
#include <curl/curl.h>
#include <queue>
#include <string>

namespace mltdl {

class DownloadManager {
public:
  DownloadManager(size_t max_concurrent_tasks = 8)
      : thread_pool_(max_concurrent_tasks), curl_pool_(max_concurrent_tasks),
        num_thread_(max_concurrent_tasks) {}
  void start(bool wait = true) { thread_pool_.executeAll(wait); };
  int download(const std::string &url, const std::string &file_dir);

private:
  void downloadFile(const std::string &url, const std::string &file_path,
                    int64_t start, int64_t end);

  int64_t fileMerge(const std::string file_path,
                    const std::vector<std::string> &temp_file_paths);

  ThreadPool thread_pool_;
  CurlPool curl_pool_;
  std::mutex mutex_;
  int num_thread_;
};
} // namespace mltdl
