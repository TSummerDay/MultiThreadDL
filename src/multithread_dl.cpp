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
  std::cout << "Usage: prog [--url url]" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "\t--url\t\t(default: \"\")" << std::endl;
}

int main(int argc, char *argv[]) {
  const auto cur_path = getCurPath();
  const auto download_dir = cur_path + "/download";
  auto success = createDir(download_dir);
  if (!success) {
    return -1;
  }
  if (argc == 1) {
    printHelp();
  } else {
    auto args = parse_args(argc, argv);
    if (args.count("--help") > 0) {
      printHelp();
      return 0;
    } else if (args.count("--url") > 0) {
      auto url = args["--url"];
      auto retry{2};
      auto num_thread{8};
      /**
       * I had a problem, when I had 8 threads open, often one thread failed to
       * call the get method and kept retrying, and when I reduced the thread
       * count to 6, I made 30 multithreaded downloads of the same url file, all
       * of which downloaded correctly.
       *
       * The most likely reason is that starting a large number of threads may
       * cause insufficient server or network resources.
       *
       * So I customized a policy that when the download failed, I would cut the
       * number of threads in half and retry the download
       */
      while (retry--) {
        DownloadManager dm(num_thread);
        num_thread /= 2;
        auto status = dm.download(url, download_dir);
        if (status >= 0) {
          break;
        }
      }
    } else {
      printHelp();
    }
  }
  return 0;
}