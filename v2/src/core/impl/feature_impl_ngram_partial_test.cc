//
// Created by Arseny Tolmachev on 2017/10/13.
//

#include "feature_impl_ngram_partial.h"
#include "core/analysis/perceptron.h"
#include "feature_impl_combine.h"
#include "testing/standalone_test.h"

using namespace jumanpp;
using namespace jumanpp::core;
using namespace jumanpp::core::analysis;
using namespace jumanpp::core::features;
using namespace jumanpp::core::features::impl;

TEST_CASE("unigram is equal to full") {
  NgramFeatureImpl<1> baseUni{0, 0};
  constexpr UnigramFeature partUni{0, 0, 0};
  util::ArraySlice<u64> data{50};
  u32 buf1[1];
  u32 buf2[1];
  baseUni.apply(buf1, data, data, data);
  partUni.step0(data, buf2);
  CHECK(buf1[0] == buf2[0]);
}

TEST_CASE("bigram is equal to full") {
  NgramFeatureImpl<2> baseUni{0, 0, 0};
  constexpr BigramFeature partUni{0, 0, 0, 0};
  util::ArraySlice<u64> data1{50};
  util::ArraySlice<u64> data2{60};
  u32 buf1[1];
  u32 buf2[1];
  u64 state1[1];
  baseUni.apply(buf1, data2, data2, data1);
  partUni.step0(data1, state1);
  partUni.step1(data2, state1, buf2);
  CHECK(buf1[0] == buf2[0]);
}

TEST_CASE("trigram is equal to full") {
  NgramFeatureImpl<3> baseUni{0, 0, 0, 0};
  constexpr TrigramFeature partUni{0, 0, 0, 0, 0};
  util::ArraySlice<u64> data1{50};
  util::ArraySlice<u64> data2{60};
  util::ArraySlice<u64> data3{70};
  u32 buf1[1];
  u32 buf2[1];
  u64 state1[1];
  u64 state2[1];
  baseUni.apply(buf1, data3, data2, data1);
  partUni.step0(data1, state1);
  partUni.step1(data2, state1, state2);
  partUni.step2(data3, state2, buf2);
  CHECK(buf1[0] == buf2[0]);
}