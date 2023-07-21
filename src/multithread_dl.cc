#include "download_manager.h"
#include "utils.h"
#include <iostream>
#include <string>
#include <vector>

using namespace mltdl;

int main(int argc, char *argv[]) {
  const auto cur_path = getCurPath();
  const auto download_dir = cur_path + "/download";
  auto success = createDir(download_dir);
  if (!success) {
    return -1;
  }

  std::cout << "Please enter the url you want to download:" << std::endl;
  std::cout << "Enter 'yes' to start, 'no' to quit!" << std::endl;
  std::string enter;
  std::vector<std::string> urls;

  DownloadManager dm;

  while (std::cin >> enter) {
    if (enter == "yes") {
      for (const auto &url : urls) {
        dm.addTask(url, download_dir);
      }
      dm.start();
      std::cout << "Download start ----------- please wait" << std::endl;
      std::cout << "enter 'q' to quit the system" << std::endl;
    } else if (enter == "no") {
      urls.clear();
    } else if (enter == "q") {
      break;
    } else {
      urls.push_back(enter);
    }
  }

  return 0;
}