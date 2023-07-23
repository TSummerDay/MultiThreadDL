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

TEST(Download, md5) {
  const auto filedir = getCurPath() + "/download";
  EXPECT_TRUE(createDir(filedir));
  const std::vector<std::string> urls = {
      "https://dl.todesk.com/windows/inst.exe",
      "https://newdl.todesk.com/linux/todesk-v4.3.1.0-amd64.deb",
      "https://down.sandai.net/thunder11/XunLeiWebSetup11.4.8.2122xl11.exe"};
  const std::vector<std::string> md5 = {"70a5bc8061e7ccd140556c6c1a0aaa93",
                                        "993ccd4a49e621855e3eceec592a6847",
                                        "c8383ce6a7d39c0a52ee906287cdb340"};
  const std::vector<std::string> filepaths = {
      filedir + "/inst.exe", filedir + "/todesk-v4.3.1.0-amd64.deb",
      filedir + "/xunlei.exe"};
  auto client = get_clients("https");
  for (size_t i = 0; i < urls.size(); ++i) {
    FILE *file = fopen(filepaths[i].c_str(), "wb");
    auto response = client->get(urls[i], rs, file);
    fclose(file);
    EXPECT_TRUE(md5[i] == calculateMd5(filepaths[i]));
  }
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
  for (auto i = 0; i < 10; ++i) {
    dm.addTask(url, filedir, true);
  }
  dm.start();
}

} // namespace mltdl