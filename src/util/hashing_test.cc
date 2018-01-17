//
// Created by Arseny Tolmachev on 2017/02/20.
//

#include "hashing.h"
#include "murmur_hash.h"
#include "testing/standalone_test.h"
#include "util/array_slice.h"

using namespace jumanpp::util::hashing;
using namespace jumanpp;

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

TEST_CASE("object api works") {
  Hasher h(1);
  auto v = h.merge(5, 6).merge(2).result();
  CHECK(v == 0x9ff35a5bb291c46fULL);
}

TEST_CASE("function api works") {
  auto v = hashCtSeq(1, 5, 6);
  CHECK(v == 0x9ff35a5bb291c46fULL);
}

TEST_CASE("indexed seq api works") {
  i32 seq[] = {7, 5, 0, 6, 2, 3};
  i32 idxes[] = {1, 3};
  auto idxSlice = util::ArraySlice<i32>{idxes};
  auto v = hashIndexedSeq(1, seq, idxSlice);
  CHECK(v == 0x9ff35a5bb291c46fULL);
}