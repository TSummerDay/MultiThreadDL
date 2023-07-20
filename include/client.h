#pragma once

#include <curl/curl.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace mltdl {

struct Response {
  long status_code{0};
  std::vector<char> body;
};

class Client {
public:
  virtual ~Client() {}

  virtual Response get(const std::string &url) = 0;
  virtual Response post(const std::string &url) = 0;
};

class HttpClient : public Client {
public:
  HttpClient();
  ~HttpClient();

  Response get(const std::string &url) override;
  Response post(const std::string &url) override;

private:
  static size_t writeCallBack(void *contents, size_t size, size_t nmemb,
                              std::vector<char> *out);
  CURL *curl_{nullptr};
};

} // namespace mltdl