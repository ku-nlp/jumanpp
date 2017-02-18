//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include <testing/standalone_test.h>
#include "darts_trie.h"

#include <util/string_piece.h>

namespace c = jumanpp::core;

TEST_CASE("darts") {
  c::DoubleArrayBuilder bldr;
  bldr.add("test", 1);
  bldr.add("tiny", 2);
  CHECK_OK(bldr.build());
  CHECK(bldr.underlyingStorage() != nullptr);
  CHECK(bldr.underlyingByteSize() > 0);
}