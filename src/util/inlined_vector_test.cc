//
// Created by Arseny Tolmachev on 2017/02/20.
//

#include "inlined_vector.h"
#include <testing/standalone_test.h>

using namespace jumanpp::util;
using namespace jumanpp;

TEST_CASE("inlinedvector works") {
  InlinedVector<int, 4> iv;
  iv.push_back(1);
  iv.push_back(2);
  iv.push_back(3);
  iv.push_back(4);
  CHECK(reinterpret_cast<u8*>(iv.data()) - reinterpret_cast<u8*>(&iv) < 100);
}