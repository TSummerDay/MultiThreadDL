#include "file_handler.h"
#include "utils.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace mltdl {

FileHandler *FileHandler::ins_ = nullptr;
std::mutex FileHandler::mutex_;

std::string FileHandler::write(const std::string &filepath,
                               const std::vector<char> &data) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::ofstream file(filepath, std::ios::binary);
  if (file.is_open()) {
    file.write(data.data(), data.size());
  }
  return filepath;
}

std::vector<char> FileHandler::read(const std::string &filepath) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::ifstream file(filepath, std::ios::binary);
  if (file.is_open()) {
    return std::vector<char>(std::istreambuf_iterator<char>(file),
                             std::istreambuf_iterator<char>());
  } else {
    return {};
  }
}

} // namespace mltdl