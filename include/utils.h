#pragma once

#include <string>

namespace mltdl {
std::string calculateMd5(const std::string &filepath);

std::string calculateSHA256(const std::string &filepath);

bool isUrlValid(const std::string &url);

std::string getProtocol(const std::string &url);

std::string adjustFilepath(const std::string &filedir, const std::string &url);

bool createDir(const std::string &dir);

void createFile(const std::string &filename);

std::string randomStrign(int n);

std::string getCurPath();

std::string getUrlName(const std::string &url);

} // namespace mltdl