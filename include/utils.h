#pragma once

#include <string>

namespace mltdl {
std::string calculateMd5(const std::string &filepath);

bool isUrlValid(const std::string &url);

std::string getProtocol(const std::string &url);

std::string adjustFilepath(const std::string &filepath);
} // namespace mltdl