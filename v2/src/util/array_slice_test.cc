//
// Created by Arseny Tolmachev on 2017/02/20.
//

#include "array_slice.h"
#include <testing/standalone_test.h>
#include <string>
#include "string_piece.h"

using namespace jumanpp::util;
using jumanpp::u8;

TEST_CASE("arrayslice works with string") {
  std::string str = "hello";
  ArraySlice<char> data{str};
  CHECK(data.length() == 5);  // 5 + zero
}