//
// Created by Arseny Tolmachev on 2017/02/18.
//

#ifndef _WIN32_WINNT
#include <unistd.h>
#endif

#include <fcntl.h>
#include <testing/standalone_test.h>
#include <fstream>

#include "mmap.h"

namespace u = jumanpp::util;

TEST_CASE("mmap correctly reads a file", "[mmap]") {
  u::MappedFile f;

  CHECK_OK(f.open("label.txt", u::MMapType::ReadOnly));
  CHECK(f.size() == 19);

  u::MappedFileFragment v;
  CHECK_OK(f.map(&v, 0, 19));
  auto bytes = static_cast<char *>(v.address());
  CHECK(bytes);
  std::string data{bytes, bytes + 19};
  CHECK(data == "some stupid content");
}

TEST_CASE("mmap correctly writes data", "[mmap]") {
  TempFile tmp;
  REQUIRE(tmp.isOk());

  auto fname = tmp.name();

  {
    u::MappedFile f;
    REQUIRE(f.open(fname, u::MMapType::ReadWrite));
    u::MappedFileFragment v;
    std::string data = "testdata";
    REQUIRE(f.map(&v, 0, data.size()));
    std::copy(std::begin(data), std::end(data),
              reinterpret_cast<char *>(v.address()));
    CHECK_OK(v.flush());
  }

  std::ifstream ifs{tmp.name()};
  std::string data;
  std::getline(ifs, data);
  CHECK(data == "testdata");
}
