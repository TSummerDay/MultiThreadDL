#include "client.h"
#include "client_factory.h"
#include "download_manager.h"
#include "file_guard.h"
#include "file_handler.h"
#include "utils.h"
#include <curl/curl.h>

#include <gtest/gtest.h>

namespace mltdl {
RetryStrategy rs{3, 500, 2};

TEST(Download, client) {
  const auto url = "https://dl.todesk.com/windows/inst.exe";
  auto protocol = getProtocol(url);
  EXPECT_TRUE(protocol == "https");
  auto client = get_clients(protocol);
  EXPECT_TRUE(client != nullptr);
  CURL *curl = curl_easy_init();
  auto file_size = client->getFileSize(url, curl);
  EXPECT_TRUE(file_size == 1550608);
  auto response = client->get(url, rs, curl, 0, file_size - 1);
  EXPECT_TRUE(response.status_code == 206);
  response = client->post("https://example.com/", "test", rs, curl);
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
  const std::vector<std::string> sha256 = {
      "f0d6288fb52a2b11193a2634de7a5e3695e26558b019115326b08a972822b543",
      "4bfaa6a5b4e8f5fe369a38658aace233b98e3214f3b6f39d482ccd73d9e847ea",
      "300a07427a07003db039d510effdabc226e0922b1d187a7846c95b241ff20c30"};
  const std::vector<int64_t> file_sizes = {1550608, 47772684, 5426224};
  const std::vector<std::string> filepaths = {
      filedir + "/inst.exe", filedir + "/todesk-v4.3.1.0-amd64.deb",
      filedir + "/xunlei.exe"};
  auto client = get_clients("https");
  CURL *curl = curl_easy_init();

  for (size_t i = 0; i < urls.size(); ++i) {
    {
      FileGuard guard(filepaths[i], "wb");
      auto file_size = client->getFileSize(urls[i], curl);
      EXPECT_TRUE(file_sizes[i] == file_size);
      auto response =
          client->get(urls[i], rs, curl, 0, file_size - 1, guard.handle());
      EXPECT_TRUE(response.status_code == 206);
    }
    EXPECT_TRUE(md5[i] == calculateMd5(filepaths[i]));
    auto sha256s = calculateSHA256(filepaths[i]);
    std::cout << sha256s << std::endl;
    EXPECT_TRUE(sha256[i] == calculateSHA256(filepaths[i]));
  }
}

TEST(Download, download_manager) {
  const std::vector<std::string> urls = {
      "https://dl.todesk.com/windows/inst.exe",
      "https://newdl.todesk.com/linux/todesk-v4.3.1.0-amd64.deb",
      "https://down.sandai.net/thunder11/XunLeiWebSetup11.4.8.2122xl11.exe"};
  const auto filedir = "download";
  DownloadManager dm(6);
  // for (size_t j = 0; j < 100; ++j) {
  for (size_t i = 0; i < urls.size(); ++i) {
    dm.download(urls[i], filedir);
  }
  // }
}

} // namespace mltdl