//
// Created by Arseny Tolmachev on 2017/07/20.
//

#include "mdic_format.h"
#include "testing/standalone_test.h"

using namespace jumanpp;

TEST_CASE("can correctly encode sequences") {
  auto checker = [](StringPiece raw, StringPiece result) {
    CAPTURE(raw);
    CAPTURE(result);
    util::io::Printer p;
    jumandic::output::fmt::printMaybeQuoted(p, raw);
    CHECK(p.result() == result);
  };
  checker("test", "test");
  checker("test,asd", "\"test,asd\"");
  checker("test\"asd", R"("test""asd")");
  checker("test\"", R"("test""")");
  checker("\"asd", R"("""asd")");
}