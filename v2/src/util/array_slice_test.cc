//
// Created by Arseny Tolmachev on 2017/02/20.
//

#include "array_slice.h"
#include <testing/standalone_test.h>
#include "string_piece.h"

using namespace jumanpp::util;

TEST_CASE("arrayslice works with string literal") {
  ArraySlice<const char> data{"hello"};
  CHECK(data.length() == 6); // 5 + zero
}

TEST_CASE("arrayslice works with stringpiece") {
  jumanpp::StringPiece piece { "hello" };
  ArraySlice<const char> data { piece };
  CHECK(data.length() == 5);
}


