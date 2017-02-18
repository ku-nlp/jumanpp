//
// Created by Arseny Tolmachev on 2017/02/18.
//


#include <testing/standalone_test.h>

#include "mmap.hpp"

namespace u = jumanpp::util;

TEST_CASE("mmap correctly reads a file", "[mmap]") {
  u::mmap_file f;

  CHECK_OK(f.open("label.txt", u::MMapType::ReadOnly));
  CHECK(f.size() == 19);

  u::mmap_view v;
  CHECK_OK(f.map(&v, 0, 19));
  auto bytes = static_cast<char*>(v.address());
  CHECK(bytes);
  std::string data {bytes, bytes + 19};
  CHECK(data == "some stupid content");
}