//
// Created by Arseny Tolmachev on 2017/02/20.
//

#include "array_slice.h"
#include <testing/standalone_test.h>
#include "string_piece.h"
#include <string>

using namespace jumanpp::util;

TEST_CASE("arrayslice works with string") {
  std::string str = "hello";
  ArraySlice<char> data{str};
  CHECK(data.length() == 5); // 5 + zero
}

TEST_CASE("arrayslice works with stringpiece") {
  jumanpp::StringPiece piece { "hello" };
  ArraySlice<char> data { piece };
  CHECK(data.length() == 5);
}


