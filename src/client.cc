#include "client.h"
#include <chrono>
#include <iostream>
#include <thread>

namespace mltdl {
HttpClient::HttpClient() : curl_(curl_easy_init()) {
  if (!curl_) {
    std::cerr << "Failed to initialize libcurl" << std::endl;
    throw std::runtime_error("Failed to initialize libcurl");
  }
}
HttpClient::~HttpClient() {
  if (curl_) {
    curl_easy_cleanup(curl_);
  }
}

Response HttpClient::get(const std::string &url, const RetryStrategy &rs,
                         void *userp /*= nullptr*/) {
  Response response;

  curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());

  if (userp != nullptr) {
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallBack2);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, userp);
  } else {
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallBack);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response.body);
  }
  // when it receives a 301 response, it automatically redirect the request to
  // the new url. However, it is worth noting that this may result in the
  // request being sent to an untrusted server.
  curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);

  // Limit the number of redirects to 10
  curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 10L);
  // Specify the redirection protocol as HTTP and HTTPS
  curl_easy_setopt(curl_, CURLOPT_REDIR_PROTOCOLS,
                   CURLPROTO_HTTP | CURLPROTO_HTTPS);

  int delay_ms = rs.delay_ms;

  for (auto i = 0; i < rs.max_retries; ++i) {
    CURLcode res = curl_easy_perform(curl_);
    if (res == CURLE_OK) {
      curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response.status_code);
      if (response.status_code >= 200 && response.status_code < 300) {
        // Request succeeded, break out of the retry loop
        break;
      } else if (response.status_code == 404) {
        // Not Found, no sense in retrying
        std::cerr << "Resource not found, no retries needed" << std::endl;
        break;
      }
    }
    // Request failed, log the failure and continue the loop
    std::cerr << "Request failed with error: " << curl_easy_strerror(res)
              << std::endl;
    std::cerr << "Retrying after " << delay_ms << " ms ..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    delay_ms *= rs.delay_factor;
  }

  return response;
}
Response HttpClient::post(const std::string &url, const RetryStrategy &rs,
                          void *userp /*= nullptr*/) {
  Response response;
  // Set the URL
  curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());

  // Set the callback function to a no-op function so libcurl does not print the
  // header
  if (userp) {
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallBack2);
  }

  // Use HTTP HEAD method
  curl_easy_setopt(curl_, CURLOPT_NOBODY, 1L);

  // Perform the request
  CURLcode res = curl_easy_perform(curl_);
  if (res != CURLE_OK) {
    throw std::runtime_error(curl_easy_strerror(res));
  }

  // Get the HTTP response code
  curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response.status_code);

  // Reset the HTTP method to HTTP GET for future requests
  curl_easy_setopt(curl_, CURLOPT_NOBODY, 0L);

  return response;
}

size_t HttpClient::writeCallBack(void *contents, size_t size, size_t nmemb,
                                 std::vector<char> *out) {
  size_t total_size = size * nmemb;
  out->insert(out->end(), (char *)contents, (char *)contents + total_size);
  return total_size;
}
size_t HttpClient::writeCallBack2(void *ptr, size_t size, size_t nmemb,
                                  void *stream) {
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

} // namespace mltdl