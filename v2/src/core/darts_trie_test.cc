//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include "darts_trie.h"
#include <testing/standalone_test.h>

namespace c = jumanpp::core;
namespace j = jumanpp;

TEST_CASE("darts builds without fail") {
  c::DoubleArrayBuilder bldr;
  bldr.add("test", 1);
  bldr.add("tiny", 2);
  CHECK_OK(bldr.build());
  CHECK(bldr.underlyingStorage() != nullptr);
  CHECK(bldr.underlyingByteSize() > 0);
}

TEST_CASE("darts builds and traverses") {
  c::DoubleArrayBuilder bldr;
  bldr.add("test", 1);
  bldr.add("tiny", 2);
  bldr.add("anchor", 3);
  bldr.add("tanya", 4);
  CHECK_OK(bldr.build());
  j::StringPiece pc = bldr.result();
  c::DoubleArray da;
  CHECK_OK(da.loadFromMemory(pc));
  auto trav = da.traversal();
  CHECK(trav.step("t") == c::TraverseStatus::NoLeaf);
  CHECK(trav.step("e") == c::TraverseStatus::NoLeaf);
  CHECK(trav.step("s") == c::TraverseStatus::NoLeaf);
  CHECK(trav.step("t") == c::TraverseStatus::Ok);
  CHECK(trav.value() == 1);
  CHECK(trav.step("x") == c::TraverseStatus::NoNode);
}