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
  thread_pool_.enqueue([this, url, filepath, large_file](int) {
    this->downloadFile(url, filepath, large_file);
  });
}

void DownloadManager::downloadFile(const std::string &url,
                                   const std::string filedir,
                                   bool large_file /*=false*/) {
  auto valid = isUrlValid(url);
  if (!valid) {
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
  if (large_file) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto adjusted_filepath = adjustFilepath(filedir);
    FILE *file = fopen(adjusted_filepath.c_str(), "wb");
    lock.unlock();
    response = client->get(url, rs, file);
  } else {
    response = client->get(url, rs);
    if (response.status_code != 200) {
      return;
    }
    FileHandler::getInstance()->write(filedir, response.body);
  }
}
} // namespace mltdl