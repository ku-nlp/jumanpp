//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include "testing/standalone_test.h"

#include <fstream>

TEST_CASE("tests are run inside correct directory") {
  std::fstream inp("label.txt");

  CHECK(inp);

  std::string str;
  std::getline(inp, str);
  CHECK(inp.eof());

  CHECK(str == "some stupid content");
}