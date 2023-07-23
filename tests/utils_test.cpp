#include "utils.h"
#include <gtest/gtest.h>

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

TEST(Utils, url) {
  for (const auto &url : g_invalid_urls) {
    EXPECT_TRUE(!isUrlValid(url));
  }
  for (const auto &url : g_valid_urls) {
    EXPECT_TRUE(isUrlValid(url));
  }
}

TEST(Utils, protocol) {
  const std::string http_url = "http://example.com";
  const std::string https_url = "https://example.com";
  const std::string ftp_url = "ftp://example.com";

  auto protocol = getProtocol(http_url);
  EXPECT_TRUE(protocol == "http");
  protocol = getProtocol(https_url);
  EXPECT_TRUE(protocol == "https");
  protocol = getProtocol(ftp_url);
  EXPECT_TRUE(protocol == "ftp");
}

TEST(Utils, urlname) {
  EXPECT_TRUE("" == getUrlName(""));
  EXPECT_TRUE(
      "linux-5.10.1.tar.xz" ==
      getUrlName(
          "https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.10.1.tar.xz"));
  EXPECT_TRUE("document.txt" ==
              getUrlName("https://example.com/files/document.txt"));
  EXPECT_TRUE("document.docx" ==
              getUrlName("https://example.com/files/document.docx"));
  EXPECT_TRUE("spreadsheet.xlsx" ==
              getUrlName("https://example.com/files/spreadsheet.xlsx"));
  EXPECT_TRUE("database.sql" ==
              getUrlName("https://example.com/files/database.sql"));
  EXPECT_TRUE("photo.jpg" ==
              getUrlName("https://example.com/images/photo.jpg"));
}

} // namespace mltdl