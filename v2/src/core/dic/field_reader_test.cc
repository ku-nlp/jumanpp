//
// Created by Arseny Tolmachev on 2017/02/20.
//

#include "field_reader.h"
#include <testing/standalone_test.h>
#include <util/array_slice.h>

using namespace jumanpp;
using namespace jumanpp::core::dic::impl;

TEST_CASE("intstoragereader fills a vector via slice") {
  u8 s[] = {5, 1, 15, 55, 12, 0, 1};
  StringPiece sp{reinterpret_cast<const char*>(s),
                 reinterpret_cast<const char*>(s + sizeof(s))};
  IntStorageReader rdr{sp};

  std::vector<i32> data;
  data.resize(5);
  util::MutableArraySlice<i32> slice{&data};
  CHECK(rdr.listAt(0).fill(slice, 5) == 5);
  CHECK(data == std::vector<i32>({1, 15, 55, 12, 0}));
}
