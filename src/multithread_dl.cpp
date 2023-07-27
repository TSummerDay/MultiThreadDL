#include "download_manager.h"
#include "utils.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace mltdl;

// a simple function to parser the command line args
using Args = std::unordered_map<std::string, std::string>;
Args parse_args(int argc, char *argv[]) {
  Args args;
  for (int i = 1; i < argc; i += 2) {
    if (i + 1 < argc) {
      args[argv[i]] = argv[i + 1];
    }
  }
  return args;
}

// A help document
void printHelp() {
  std::cout << "Usage: prog [--url url] [--download_dir download_dir] "
               "[--download_path download_path]"
            << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "\t--url\t\t(default: \"\")" << std::endl;
  std::cout << "\t--download_dir\t\t (default: download)" << std::endl;
  std::cout << "\t--download_path\t\t (default: url name)" << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
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
          dm.addTask(url, download_dir, true);
        }
        dm.start(false);
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
  } else {
    auto args = parse_args(argc, argv);
    if (args.count("--help") > 0) {
      printHelp();
      return 0;
    }
  }

  // The thread pool waits for all tasks to complete before exiting the program
  return 0;
}