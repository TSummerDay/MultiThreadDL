#include "download_manager.h"
#include "client.h"
#include "client_factory.h"
#include "file_handler.h"
#include "utils.h"

namespace mltdl {

void DownloadManager::addTask(const std::string &url,
                              const std::string &filepath) {
  thread_pool_.enqueue(
      [this, url, filepath](int) { this->downloadFile(url, filepath); });
}

void DownloadManager::downloadFile(const std::string &url,
                                   const std::string filepath) {
  auto valid = isUrlValid(url);
  if (!valid) {
    return;
  }
  auto protocol = getProtocol(url);
  auto client = get_clients(protocol);
  auto response = client->get(url);
  if (response.status_code != 200) {
    return;
  }
  FileHandler::getInstance()->write(filepath, response.body);
}
} // namespace mltdl