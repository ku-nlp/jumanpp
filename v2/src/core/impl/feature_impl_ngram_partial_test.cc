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

NgramFeature nf(i32 index, std::initializer_list<i32> data) {
  NgramFeature f;
  f.index = index;
  f.arguments.assign(data);
  return f;
}

TEST_CASE("partial and full trigram features produce the same result") {
  NgramDynamicFeatureApply full;
  PartialNgramDynamicFeatureApply part;
  int idx = 0;
  auto add = [&](std::initializer_list<i32> data) {
    auto f = nf(idx++, data);
    full.addChild(f);
    part.addChild(f);
  };
  add({0});
  add({1});
  add({0, 1});
  add({1, 0});
  add({1, 1, 1});
  add({0, 0, 0});
  util::ArraySlice<u64> t0data{0, 1, 2, 0, 1, 2};
  util::ArraySlice<u64> t1data{5, 2};
  util::ArraySlice<u64> t2data{
      2, 1,
  };
  u32 resultFullBuf[6 * 3];
  u32 resultPartBuf[2 * 3];
  util::Sliceable<u32> fullSlice{resultFullBuf, 6, 3};
  util::ConstSliceable<u64> t0slice{t0data, 2, 3};
  NgramFeatureData nfd{fullSlice, t2data, t1data, t0slice};
  full.applyBatch(&nfd);

  u32 partRes[6];
  util::Sliceable<u32> partSlice{partRes, 2, 3};
  part.applyUni(t0slice, partSlice);

  CHECK(partSlice.at(0, 0) == fullSlice.at(0, 0));
  CHECK(partSlice.at(1, 0) == fullSlice.at(1, 0));
  CHECK(partSlice.at(2, 0) == fullSlice.at(2, 0));
  CHECK(partSlice.at(0, 1) == fullSlice.at(0, 1));
  CHECK(partSlice.at(1, 1) == fullSlice.at(1, 1));
  CHECK(partSlice.at(2, 1) == fullSlice.at(2, 1));

  u64 biState[6];
  util::Sliceable<u64> biStateSlice{biState, 2, 3};
  part.applyBiStep1(t0slice, biStateSlice);
  part.applyBiStep2(biStateSlice, t1data, partSlice);
  CHECK(partSlice.at(0, 0) == fullSlice.at(0, 2));
  CHECK(partSlice.at(1, 0) == fullSlice.at(1, 2));
  CHECK(partSlice.at(2, 0) == fullSlice.at(2, 2));
  CHECK(partSlice.at(0, 1) == fullSlice.at(0, 3));
  CHECK(partSlice.at(1, 1) == fullSlice.at(1, 3));
  CHECK(partSlice.at(2, 1) == fullSlice.at(2, 3));

  u64 triState1[6];
  u64 triState2[6];
  util::Sliceable<u64> triStateSlice1{triState1, 2, 3};
  util::Sliceable<u64> triStateSlice2{triState2, 2, 3};
  part.applyTriStep1(t0slice, triStateSlice1);
  part.applyTriStep2(triStateSlice1, t1data, triStateSlice2);
  part.applyTriStep3(triStateSlice2, t2data, partSlice);
  CHECK(partSlice.at(0, 0) == fullSlice.at(0, 4));
  CHECK(partSlice.at(1, 0) == fullSlice.at(1, 4));
  CHECK(partSlice.at(2, 0) == fullSlice.at(2, 4));
  CHECK(partSlice.at(0, 1) == fullSlice.at(0, 5));
  CHECK(partSlice.at(1, 1) == fullSlice.at(1, 5));
  CHECK(partSlice.at(2, 1) == fullSlice.at(2, 5));
}