//
// Created by Arseny Tolmachev on 2017/02/18.
//

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include "test_config.h"

#ifdef _WIN32_WINNT
#include <direct.h>
#define chdir _chdir
#endif

int main(int argc, char* argv[]) {
  std::printf("using %s as test resource directory...", TEST_RESOURCE_DIR);

  if (chdir(TEST_RESOURCE_DIR) != 0) {
    std::printf(" failed: %s", strerror(errno));
  } else {
    std::puts("\n");
  }

  int result = Catch::Session().run(argc, argv);
  return (result < 0xff ? result : 0xff);
}
