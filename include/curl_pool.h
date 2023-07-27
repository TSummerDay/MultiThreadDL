#pragma once

#include <curl/curl.h>
#include <mutex>
#include <queue>

namespace mltdl {

/**
 * define a CURL pool to assign CURL handles to each thread
 * this allows you to reuse connections that CURL has already made
 * */

class CurlPool {
public:
  CurlPool(int num_curl);

  ~CurlPool();

  CURL *acquire();
  void release(CURL *curl);

private:
  std::queue<CURL *> curls_;
  std::mutex mutex_;
};

/**
 * defines an automatic management class for the curl handle
 * there is no need to manually go to the queue to get the curl handle and put
 * the curl handle in the queue.
 * because a function can end up anywhere
 */
class CurlGuard {
public:
  CurlGuard(CurlPool &pool);
  ~CurlGuard();

  // return the CURL* handle
  CURL *handle() const;

  // disallow copy operations
  // make sure that no two classes release the same curl handle
  CurlGuard(const CurlGuard &) = delete;
  CurlGuard &operator=(const CurlGuard &) = delete;

private:
  CurlPool &pool_;
  CURL *curl_;
};

} // namespace mltdl