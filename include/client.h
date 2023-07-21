#pragma once

#include <curl/curl.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace mltdl {

struct Response {
  long status{0};
  long status_code{0};
  std::string content_type;
  std::vector<char> body;
};

struct RetryStrategy {
  int max_retries{0};
  int delay_ms{0};
  int delay_factor{1};
};

class Client {
public:
  virtual ~Client() {}

  // Define a callback function to download a large amount of data in batches
  using CallBack = std::function<size_t(void *, size_t, size_t, void *)>;

  virtual Response get(const std::string &url, const RetryStrategy &rs,
                       void *userp = nullptr) = 0;
  virtual Response post(const std::string &url, const RetryStrategy &rs,
                        void *userp = nullptr) = 0;
};

class HttpClient : public Client {
public:
  HttpClient();
  ~HttpClient();

  Response get(const std::string &url, const RetryStrategy &rs,
               void *userp = nullptr) override;
  Response post(const std::string &url, const RetryStrategy &rs,
                void *userp = nullptr) override;

private:
  // declare the callback function as static in multithread
  // Byte stream is loaded into memory
  static size_t writeCallBack(void *contents, size_t size, size_t nmemb,
                              std::vector<char> *out);

  // Fetch byte streams in batches and write them to disk
  static size_t writeCallBack2(void *ptr, size_t size, size_t nmemb,
                               void *stream);
  CURL *curl_{nullptr};
};

} // namespace mltdl