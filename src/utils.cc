#include "utils.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <openssl/md5.h>
#include <regex>
#include <sstream>
#include <vector>

namespace mltdl {

std::string calculateMd5(const std::string &filepath) {
  std::ifstream file(filepath, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file");
  }

  std::vector<char> buffer(MD5_DIGEST_LENGTH);
  MD5_CTX md5_context;
  MD5_Init(&md5_context);

  char buf[1024 * 16];
  while (file.read(buf, sizeof(buf))) {
    MD5_Update(&md5_context, buf, file.gcount());
  }

  // handle the last block of bytes
  MD5_Update(&md5_context, buf, file.gcount());

  unsigned char result[MD5_DIGEST_LENGTH];
  MD5_Final(result, &md5_context);

  std::ostringstream oss;
  for (auto i = 0; i < MD5_DIGEST_LENGTH; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(result[i]);
  }
  return oss.str();
}

bool isUrlValid(const std::string &url) {
  std::regex g_url_regex(
      R"(^(http|https|ftp)://([a-z0-9.-]+)(:[0-9]+)?(/[\w-./?%&=]*)?$)");
  if (std::regex_match(url, g_url_regex)) {
    return true;
  } else {
    return false;
  }
}

std::string getProtocol(const std::string &url) {
  std::size_t protocol_end_pos = url.find("://");
  if (protocol_end_pos != std::string::npos) {
    auto protocol = url.substr(0, protocol_end_pos);
    std::transform(protocol.begin(), protocol.end(), protocol.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return protocol;
  } else {
    throw std::runtime_error("URL does not contain a protocol");
  }
}

std::string adjustFilepath(const std::string &filepath) {
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
