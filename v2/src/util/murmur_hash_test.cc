//
// Created by Arseny Tolmachev on 2017/02/20.
//

#include "murmur_hash.h"
#include <testing/standalone_test.h>

using namespace jumanpp::util::hashing;

TEST_CASE("murmurhash compiles with ints") {
  auto a1 = murmur_combine(100, 50);
  CHECK(a1 == 13165272538622257426ULL);
}

TEST_CASE("murmur hash works with strings") {
  auto string = "this is a test string";
  auto ptr = reinterpret_cast<const jumanpp::u8*>(string);
  auto hv = murmurhash3_memory(ptr, ptr + sizeof(string), 10);
  CHECK(hv == 0x2e62c68031e4659dULL);
}