#include "client.h"
#include "client_factory.h"
#include "download_manager.h"
#include "file_handler.h"
#include "utils.h"

#include <gtest/gtest.h>

namespace mltdl {
RetryStrategy rs{3, 500, 2};

TEST(Download, client) {
  const auto url = "https://www.kuaidi100.com/";
  auto protocol = getProtocol(url);
  auto client = get_clients(protocol);
  auto response = client->get(url, rs);
  EXPECT_TRUE(response.status_code == 200);
  response = client->post(url, rs);
  EXPECT_TRUE(response.status_code == 200);
}

TEST(Download, file_handler) {
  const auto url = "https://www.kuaidi100.com/";
  const auto filepath = "download/test.txt";
  auto protocol = getProtocol(url);
  auto client = get_clients(protocol);
  auto response = client->get(url, rs);
  EXPECT_TRUE(response.status_code == 200);
  FileHandler::getInstance()->write(filepath, response.body);
  auto data = FileHandler::getInstance()->read(filepath);
  EXPECT_TRUE(data == response.body);
}

TEST(Download, download_manager) {
  const auto url = "https://www.kuaidi100.com/";
  const auto filedir = "download";
  DownloadManager dm;
  for (auto i = 0; i < 100; ++i) {
    dm.addTask(url, filedir, true);
  }
  dm.start();
}

} // namespace mltdl