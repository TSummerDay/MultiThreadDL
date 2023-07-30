#include "download_manager.h"
#include "client.h"
#include "client_factory.h"
#include "file_guard.h"
#include "file_handler.h"
#include "utils.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace mltdl {

/**
 * 0: the download failed, but normal exit does not require retry
 * 1: the download success
 * -1: there is probably a network problem and it is worth trying again
 *
 * to be more reasonable, it should define a Status class to represent different
 * states
 */
int DownloadManager::download(const std::string &url,
                              const std::string &file_dir) {
  CurlGuard guard(curl_pool_);
  auto curl = guard.handle();
  if (url.empty()) {
    std::cout << "url is empty!" << std::endl;
    return 0;
  }
  auto valid = isUrlValid(url);
  if (!valid) {
    std::cout << url << " url is invalid!" << std::endl;
    return 0;
  }
  auto protocol = getProtocol(url);
  auto client = get_clients(protocol);
  if (curl == nullptr) {
    return 0;
  }
  auto file_size = client->getFileSize(url, curl);
  if (file_size < 0) {
    /**
     * If the file size of the resource cannot be obtained, do I need to return
     * to single-threaded download?
     *
     * If the size of the obtained file does not match the actual size, and it
     * does not have Content-MD5 or Content-SHA in the request header, the
     * download fails
     *
     * We may need to manually verify the MD5 and SHA of the resource after the
     * download is complete
     */
    return -1;
  }
  auto part_size = file_size / num_thread_;
  auto file_path = adjustFilepath(file_dir, url);
  createFile(file_path);
  std::vector<std::string> temp_file_paths(num_thread_);
  for (auto i = 0; i < num_thread_; ++i) {
    auto temp_file_path = adjustFilepath(file_dir, url);
    temp_file_paths[i] = temp_file_path;
    createFile(temp_file_path);
  }
  for (auto i = 0; i < num_thread_; ++i) {
    int64_t start = i * part_size;
    int64_t end = ((i + 1) * part_size) - 1;
    if (i == num_thread_ - 1) {
      end = file_size - 1;
    }
    thread_pool_.enqueue([this, url, start, end, temp_file_paths, i](int) {
      this->downloadFile(url, temp_file_paths[i], start, end);
    });
  }
  // start all task and wait them complete
  std::cout << "Download start, please wait ---------" << std::endl;
  start();
  auto merge_size = fileMerge(file_path, temp_file_paths);
  if (merge_size != file_size) {
    std::cerr << "merge file failed" << std::endl;
    std::remove(file_path.c_str());
    return -1;
  }
  auto md5 = calculateMd5(file_path);
  auto sha256 = calculateSHA256(file_path);
  std::cout << "md5: " << md5 << std::endl;
  std::cout << "sha256: " << sha256 << std::endl;
  std::cout << "file save to :" << file_path << std::endl;
  return 1;
}

void DownloadManager::downloadFile(const std::string &url,
                                   const std::string &file_path, int64_t start,
                                   int64_t end) {
  CurlGuard guard(curl_pool_);
  FileGuard file_guard(file_path, "wb");
  auto curl = guard.handle();
  auto file = file_guard.handle();
  if (!file) {
    std::cerr << "file open failed" << file_path << std::endl;
    return;
  }
  auto protocol = getProtocol(url);
  auto client = get_clients(protocol);
  if (client == nullptr) {
    std::cerr << "Download file failed" << std::endl;
    return;
  }
  Response response;
  RetryStrategy rs{3, 500, 2};

  response = client->get(url, rs, curl, start, end, file);
}

/**
 * when I was working on the file merge operation, I discovered a problem
 * I use FileGuard class to create these files and write data in them , then I
 * read these file by std::ifstream, and write the target file by std::ofstream,
 * I find the target file usualy less than the sum of these files.
 * The most likely reason is that the end character (EOF) is different when
 * reading and writing binary files at the bottom
 *
 * Through subsequent learning, I understand that this is because I use
 * while(file.read(buffer, sizeof(buffer)) when reading a file with
 * std::ifstream, and std::ifstream::read() after reading a specified byte, If
 * the End of file (EOF) is encountered, the read will also stop. But that
 * doesn't mean read() didn't read any data. Therefore, at the end of the loop,
 * file.eof() should be judged to be true, and the number of bytes last read
 * should be written.
 */

int64_t
DownloadManager::fileMerge(const std::string file_path,
                           const std::vector<std::string> &temp_file_paths) {
  FileGuard output_file(file_path, "wb");
  int64_t merge_size = 0;
  if (!output_file.handle()) {
    std::cerr << "Failed to open output file" << std::endl;
    return merge_size;
  }
  char buffer[1024];
  for (auto i = 0; i < num_thread_; ++i) {
    FileGuard input_file(temp_file_paths[i], "rb");
    if (!input_file.handle()) {
      std::cerr << "Failed to open input file: " << temp_file_paths[i]
                << std::endl;
      continue;
    }
    while (!feof(input_file.handle())) {
      size_t read_size = fread(buffer, 1, sizeof(buffer), input_file.handle());
      if (ferror(input_file.handle())) {
        std::cerr << "Error reading input file: " << temp_file_paths[i]
                  << std::endl;
        break;
      }
      merge_size += read_size;
      fwrite(buffer, 1, read_size, output_file.handle());
      if (ferror(output_file.handle())) {
        std::cerr << "Error writing to output file" << std::endl;
        break;
      }
    }
    std::remove(temp_file_paths[i].c_str());
  }
  return merge_size;
}
} // namespace mltdl