#include "client.h"
#include "client_factory.h"
#include "download_manager.h"
#include "file_handler.h"
#include "utils.h"

#include <gtest/gtest.h>
#include <regex>

namespace mltdl {

const auto g_invalid_urls = {"http:example.com",
                             "xyz:// example.com",
                             "http:// exa<mple.com",
                             "http:// exa mple.com",
                             "http:// exa..mple.com",
                             "http:// example",
                             "http:// example.com/abc.html#",
                             "http:// example.com/abc.html#part1#part2",
                             "http:// example.com:70000",
                             "http:// example.com:abc",
                             "http:// 123",
                             "http://@@@@",
                             "http:// httpexample.com",
                             "http:// http://example.com",
                             "http://"};

const auto g_valid_urls = {
    "http://example.com",
    "https://example.com",
    "ftp://example.com",
    "http://example.com:8080",
    "http://subdomain.example.com",
    "http://example.com/path/to/resource",
    "http://example.com/path?param1=value1&param2=value2",
    "http://192.0.2.0",
    "http://example.com/path%20with%20spaces",
    "http://example.com:1234",
    "http://example.com/a/very/long/path/that/keeps/going/and/going/and/going"};

TEST(Download, url) {
  for (const auto &url : g_invalid_urls) {
    EXPECT_TRUE(!isUrlValid(url));
  }
  for (const auto &url : g_valid_urls) {
    EXPECT_TRUE(isUrlValid(url));
  }
}

TEST(Download, client) {
  const auto url = "https://www.kuaidi100.com/";
  RetryStrategy rs{3, 500, 2};
  auto protocol = getProtocol(url);
  auto client = get_clients(protocol);
  auto response = client->get(url, rs);
  EXPECT_TRUE(response.status_code == 200);
  response = client->post(url, rs);
  EXPECT_TRUE(response.status_code == 200);
}

TEST(Download, file_handler) {
  const auto url = "https://www.kuaidi100.com/";
  const auto filepath = "test.txt";
  RetryStrategy rs{3, 500, 2};
  auto protocol = getProtocol(url);
  auto client = get_clients(protocol);
  auto response = client->get(url, rs);
  EXPECT_TRUE(response.status_code == 200);
  for (auto i = 0; i < 100; i++) {
    auto adjust_path =
        FileHandler::getInstance()->write(filepath, response.body);
    auto data = FileHandler::getInstance()->read(adjust_path);
    EXPECT_TRUE(data == response.body);
  }
}

TEST(Download, download_manager) {
  const auto url = "https://www.kuaidi100.com/";
  const auto filepath = "test.txt";
  DownloadManager dm;
  for (auto i = 0; i < 100; ++i) {
    dm.addTask(url, filepath, true);
  }
  dm.start();
}

} // namespace mltdl