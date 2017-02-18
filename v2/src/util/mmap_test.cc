//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include <fcntl.h>
#include <testing/standalone_test.h>
#include <unistd.h>
#include <fstream>

#include "mmap.hpp"

namespace u = jumanpp::util;

TEST_CASE("mmap correctly reads a file", "[mmap]") {
  u::mmap_file f;

  CHECK_OK(f.open("label.txt", u::MMapType::ReadOnly));
  CHECK(f.size() == 19);

  u::mmap_view v;
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
    u::mmap_file f;
    CHECK_OK(f.open(fname, u::MMapType::ReadWrite));
    u::mmap_view v;
    std::string data = "testdata";
    f.map(&v, 0, data.size());
    std::copy(std::begin(data), std::end(data),
              reinterpret_cast<char *>(v.address()));
    CHECK_OK(v.flush());
  }

  std::ifstream ifs{tmp.name()};
  std::string data;
  std::getline(ifs, data);
  CHECK(data == "testdata");
}