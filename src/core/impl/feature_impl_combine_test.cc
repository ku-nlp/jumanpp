//
// Created by Arseny Tolmachev on 2017/03/03.
//

#include "feature_impl_combine.h"
#include "testing/standalone_test.h"

using namespace jumanpp::core::features::impl;
using namespace jumanpp;

TEST_CASE("ngram features can be compiled") {
  constexpr NgramFeatureImpl<1> uni{0, 1};
  constexpr NgramFeatureImpl<2> bi{1, 1, 2};
  constexpr NgramFeatureImpl<3> tri{2, 1, 2, 0};
  u32 data[] = {0, 0, 0};
  u64 inputs[] = {1, 1, 1};
  util::MutableArraySlice<u32> res{data};
  util::ArraySlice<u64> t2{inputs};
  util::ArraySlice<u64> t1{inputs};
  util::ArraySlice<u64> t0{inputs};
  uni.apply(&res, t2, t1, t0);
  bi.apply(&res, t2, t1, t0);
  tri.apply(&res, t2, t1, t0);
  CHECK(data[0] == 0xb43cdad6);
  CHECK(data[1] == 0x66f322e1);
  CHECK(data[2] == 0x99518c1f);
}