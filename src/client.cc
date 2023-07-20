#include "client.h"

namespace mltdl {
HttpClient::HttpClient() : curl_(curl_easy_init()) {
  if (!curl_) {
    throw std::runtime_error("Failed to initialize libcurl");
  }
}
HttpClient::~HttpClient() {
  if (curl_) {
    curl_easy_cleanup(curl_);
  }
}

Response HttpClient::get(const std::string &url) {
  Response response;

  curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());

  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallBack);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response.body);

  // when it receives a 301 response, it automatically redirect the request to
  // the new url. However, it is worth noting that this may result in the
  // request being sent to an untrusted server.
  curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);

  // Limit the number of redirects to 10
  curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 10L);
  // Specify the redirection protocol as HTTP and HTTPS
  curl_easy_setopt(curl_, CURLOPT_REDIR_PROTOCOLS,
                   CURLPROTO_HTTP | CURLPROTO_HTTPS);

  CURLcode res = curl_easy_perform(curl_);
  if (res != CURLE_OK) {
    throw std::runtime_error(curl_easy_strerror(res));
  }
  curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response.status_code);

  return response;
}
Response HttpClient::post(const std::string &url) {
  Response response;
  // Set the URL
  curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());

  // Set the callback function to a no-op function so libcurl does not print the
  // header
  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION,
                   [](void *ptr, size_t size, size_t nmemb,
                      void *userdata) -> size_t { return size * nmemb; });

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
} // namespace mltdl