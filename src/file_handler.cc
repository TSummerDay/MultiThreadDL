#include "file_handler.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace mltdl {

FileHandler *FileHandler::ins_ = nullptr;
std::mutex FileHandler::mutex_;

std::string FileHandler::write(const std::string &filepath,
                               const std::vector<char> &data) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto adjusted_filepath = adjustFilepath(filepath);
  std::ofstream file(adjusted_filepath, std::ios::binary);
  if (file.is_open()) {
    file.write(data.data(), data.size());
  }
  return adjusted_filepath;
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

std::string FileHandler::adjustFilepath(const std::string &filepath) {
  if (!std::filesystem::exists(filepath)) {
    return filepath;
  }

  std::filesystem::path p(filepath);
  auto filename_base = p.stem().string();
  auto extension = p.extension().string();

  for (auto i = 1;; ++i) {
    std::ostringstream oss;
    oss << filename_base << '(' << i << ')' << extension;
    auto new_filepath = p.parent_path().string() + oss.str();
    if (!std::filesystem::exists(new_filepath)) {
      return new_filepath;
    }
  }
}

} // namespace mltdl