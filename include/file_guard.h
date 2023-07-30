#pragma once
#include <iostream>

namespace mltdl {
class FileGuard {
public:
  FileGuard(const std::string &path, const std::string &mode) {
    file_ = fopen(path.c_str(), mode.c_str());
  }
  ~FileGuard() {
    if (file_) {
      fclose(file_);
    }
  }
  FILE *handle() { return file_; }

  FileGuard(const FileGuard &) = delete;
  FileGuard &operator=(const FileGuard &) = delete;

private:
  FILE *file_;
};
} // namespace mltdl