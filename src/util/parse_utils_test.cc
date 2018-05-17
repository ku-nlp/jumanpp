//
// Created by Arseny Tolmachev on 2018/05/16.
//

#include "parse_utils.h"
#include "testing/standalone_test.h"

namespace u = jumanpp::util;
using jumanpp::StringPiece;

TEST_CASE("parseU64 works") {
  auto doTest = [](StringPiece val, jumanpp::u64 res, bool request = true) {
    CAPTURE(val);
    CAPTURE(res);
    jumanpp::u64 v = 0;
    CHECK(u::parseU64(val, &v) == request);
    if (request) {
      CHECK(v == res);
    }
  };
  doTest("0", 0);
  doTest("15", 15);
  doTest("512412", 512412);
  doTest("a", 0, false);
  doTest("", 0, false);
  doTest(" 12", 0, false);
  doTest("12 ", 0, false);
}
