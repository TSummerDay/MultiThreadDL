#include "download_manager.h"
#include "client.h"
#include "client_factory.h"
#include "file_handler.h"
#include "utils.h"

#include <filesystem>
#include <iostream>

namespace mltdl {

void DownloadManager::addTask(const std::string &url,
                              const std::string &filedir,
                              bool large_file /*=false*/) {
  thread_pool_.enqueue([this, url, filedir, large_file](int) {
    this->downloadFile(url, filedir, large_file);
  });
}

void DownloadManager::downloadFile(const std::string &url,
                                   const std::string filedir,
                                   bool large_file /*=false*/) {
  if (url.empty()) {
    std::cout << "url is empty!" << std::endl;
    return;
  }
  auto valid = isUrlValid(url);
  if (!valid) {
    std::cout << url << " url is invalid!" << std::endl;
    return;
  }
  auto protocol = getProtocol(url);
  auto client = get_clients(protocol);
  if (client == nullptr) {
    std::cerr << "Download file failed, file path : " << filedir << std::endl;
    return;
  }
  Response response;
  RetryStrategy rs{3, 500, 2};
  // Lock when create the file on the disk.
  // Prevents multiple threads from writing to the same file simultaneously
  std::unique_lock<std::mutex> lock(mutex_);
  auto adjusted_filepath = adjustFilepath(filedir, url);
  FILE *file = fopen(adjusted_filepath.c_str(), "wb");
  lock.unlock();

  if (large_file) {
    response = client->get(url, rs, file);
  } else {
    response = client->get(url, rs);
    if (response.status_code == 200) {
      size_t written = fwrite(response.body.data(), sizeof(char),
                              response.body.size(), file);
      if (written < response.body.size()) {
        std::cerr << "write file to disk error" << std::endl;
      }
    }
  }
  fclose(file);
  std::cout << "save file to" << adjusted_filepath << std::endl;
}
} // namespace mltdl