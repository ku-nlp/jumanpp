//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include "printer.h"
#include <testing/standalone_test.h>

using namespace jumanpp::util::io;

TEST_CASE("printer prints strings") {
  Printer p;
  p << "test";
  CHECK(p.result() == "test");
}

TEST_CASE("printer prints ints") {
  Printer p;
  p << 25123;
  CHECK(p.result() == "25123");
}

TEST_CASE("printer works with indents") {
  Printer p;
  p << 5;
  {
    Indent i0{p, 2};
    p << "\n2";
  }
  p << "\n5";
  CHECK(p.result() == "5\n  2\n5");
}