#include "curl_pool.h"

namespace mltdl {
CurlPool::CurlPool(int num_curl) {
  for (auto i = 0; i < num_curl; ++i) {
    CURL *curl = curl_easy_init();
    curls_.push(curl);
  }
}

CurlPool::~CurlPool() {
  while (!curls_.empty()) {
    curl_easy_cleanup(curls_.front());
    curls_.pop();
  }
}

/**
 * make sure that each thread has a CURL handle available, even if the
 * number of threads equals the size of the CURL queue.
 * In extreme cases, the thread may not return the CURL handle to the handle
 * pool because of an exception or error.
 */
CURL *CurlPool::acquire() {
  std::loack_guard<std::mutex> lock(mutex_);
  if (curls_.empty()) {
    return curl_easy_init();
  }
  CURL *curl = curls_.front();
  curls.pop();
  return curl;
}

void CurlPool::release(CURL *curl) {
  std::lock_guard<std::mutex> lock(mutex_);
  curls_.push(curl);
}

CurlGuard::CurlGuard(CurlPool &pool) : pool_(pool), curl_(pool.acquire()) {}
CurlGuard::~CurlGuard() {
  if (curl_) {
    pool_.release(curl_);
  }
};

CURL *CurlGuard::handle() const { return curl_; }

} // namespace mltdl