#include "client.h"
#include <chrono>
#include <iostream>
#include <thread>

namespace mltdl {

// define a structure to verify that the size of the downloaded data is as
// expected
struct WriteData {
  FILE *file;
  int64_t expected_size{0};
  int64_t actual_size{0};
};

HttpClient::HttpClient() {}
HttpClient::~HttpClient() {}

// Instead of checking that end must be greater than or equal to start, we check
// before calling the get method
Response HttpClient::get(const std::string &url, const RetryStrategy &rs,
                         CURL *curl, int64_t start, int64_t end,
                         void *userp /*= nullptr*/) {
  Response response;
  WriteData write_data;

  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  if (userp != nullptr) {
    write_data.file = (FILE *)userp;
    // add 1 to include both the start and end bytes
    // even if both start and end are 0
    write_data.expected_size = end - start + 1;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallBack2);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_data);
  } else {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallBack);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
  }
  // when it receives a 301 response, it automatically redirect the request to
  // the new url. However, it is worth noting that this may result in the
  // request being sent to an untrusted server.
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  char range[64];
  snprintf(range, sizeof(range), "%ld-%ld", start, end);
  curl_easy_setopt(curl, CURLOPT_RANGE, range);
  // Limit the number of redirects to 10
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
  // Specify the redirection protocol as HTTP and HTTPS
  curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS,
                   CURLPROTO_HTTP | CURLPROTO_HTTPS);

  int delay_ms = rs.delay_ms;

  for (auto i = 0; i < rs.max_retries; ++i) {
    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status_code);
      // curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE,
      // &response.content_type);
      if (response.status_code >= 200 && response.status_code < 300) {
        if (write_data.expected_size != write_data.actual_size) {
          std::cerr
              << "The response is correct, but the request size is incorrect"
              << std::endl;
        }
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
Response HttpClient::post(const std::string &url,
                          const std::string &post_fields,
                          const RetryStrategy &rs, CURL *curl,
                          void *userp /*= nullptr*/) {
  Response response;
  WriteData write_data;

  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  // Enable the POST method
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  // Set POST fields
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());

  if (userp != nullptr) {
    write_data.file = (FILE *)userp;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallBack2);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, userp);
  } else {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallBack);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
  }

  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
  curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS,
                   CURLPROTO_HTTP | CURLPROTO_HTTPS);

  int delay_ms = rs.delay_ms;

  for (auto i = 0; i < rs.max_retries; ++i) {
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status_code);
      if (response.status_code >= 200 && response.status_code < 300) {
        break;
      } else if (response.status_code == 404) {
        std::cerr << "Resource not found, no retries needed" << std::endl;
        break;
      }
    }
    std::cerr << "Request failed with error: " << curl_easy_strerror(res)
              << std::endl;
    std::cerr << "Retrying after " << delay_ms << " ms ..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    delay_ms *= rs.delay_factor;
  }

  return response;
}

int64_t HttpClient::getFileSize(const std::string &url, CURL *curl) {
  double file_size{0.0};
  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  // make a HEAD request
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);

  CURLcode res = curl_easy_perform(curl);
  if (res == CURLE_OK) {
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &file_size);
  } else {
    std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
              << std::endl;
    return -1.0;
  }
  return static_cast<int64_t>(file_size);
}

size_t HttpClient::writeCallBack(void *contents, size_t size, size_t nmemb,
                                 std::vector<char> *out) {
  size_t total_size = size * nmemb;
  out->insert(out->end(), (char *)contents, (char *)contents + total_size);
  return total_size;
}
size_t HttpClient::writeCallBack2(void *ptr, size_t size, size_t nmemb,
                                  void *stream) {
  WriteData *write_data = (WriteData *)stream;
  size_t written = fwrite(ptr, size, nmemb, write_data->file);
  write_data->actual_size += written;
  return written;
}

} // namespace mltdl