#pragma once

#include <mutex>
#include <string>
#include <vector>

namespace mltdl {
class FileHandler {
public:
  static FileHandler *getInstance() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (ins_ == nullptr) {
      ins_ = new FileHandler();
    }
    return ins_;
  }
  std::string write(const std::string &filepath, const std::vector<char> &data);
  std::vector<char> read(const std::string &filepath);

private:
  //
  FileHandler(){};
  ~FileHandler(){};
  static FileHandler *ins_;
  static std::mutex mutex_;

  std::string adjustFilepath(const std::string &filepath);
};

} // namespace mltdl