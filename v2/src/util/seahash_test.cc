//
// Created by Arseny Tolmachev on 2017/10/13.
//

#include "seahash.h"
#include <random>
#include "testing/standalone_test.h"

namespace h = jumanpp::util::hashing;
using jumanpp::u64;

TEST_CASE("seahash values are same for full and partial") {
  auto val1 = h::seaHashSeq(1, 2, 3);
  auto s1 = h::rawSeahashStart(3, 1);
  auto s2 = h::rawSeahashContinue(s1, 2);
  auto val2 = h::rawSeahashFinish(s2, 3);
  CHECK(val1 == val2);
}

TEST_CASE("seahash values are same for full and partial (multiple values)") {
  std::minstd_rand rng{std::random_device()()};
  std::uniform_int_distribution<u64> dst{};
  for (int i = 0; i < 1000; ++i) {
    auto v1 = dst(rng);
    auto v2 = dst(rng);
    auto v3 = dst(rng);
    CAPTURE(v1);
    CAPTURE(v2);
    CAPTURE(v3);
    auto val1 = h::seaHashSeq(v1, v2, v3);
    auto s1 = h::rawSeahashStart(3, v1);
    auto s2 = h::rawSeahashContinue(s1, v2);
    auto val2 = h::rawSeahashFinish(s2, v3);
    CHECK(val1 == val2);
  }
}