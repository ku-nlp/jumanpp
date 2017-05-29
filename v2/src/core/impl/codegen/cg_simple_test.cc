//
// Created by Arseny Tolmachev on 2017/05/29.
//

#include <array>
#include "testing/test_analyzer.h"
#include "core/impl/feature_impl_combine.h"
#include "cg_2_spec.h"
#include "cgtest02.h"

using namespace jumanpp::testing;
using namespace jumanpp::core::spec::dsl;
using namespace jumanpp;

constexpr size_t NumNgrams = 13;
constexpr size_t NumExamples = 5;
constexpr size_t NumFeatures = 5;

template <typename T, size_t N>
util::Sliceable<T> slice(std::array<T, N>* slc, size_t rows) {
  util::MutableArraySlice<T> mas{slc->data(), N};
  return util::Sliceable<T>{mas, slc->size() / rows, rows};
}

struct BenchInput {
  std::array<u32, NumNgrams * NumExamples> result;
  std::array<u64, NumFeatures * NumExamples> t0;
  std::array<u64, NumFeatures> t1;
  std::array<u64, NumFeatures> t2;

  BenchInput() {
    for (int f = 0 ; f < NumFeatures; ++f) {
      for (int ex = 0; ex < NumExamples; ++ex) {
        t0.at(f + ex * NumFeatures) = 10000 + 1000 * f + ex * 2;
      }
      t1.at(f) = 20000 + f * 2;
      t2.at(f) = 30000 + f * 2;
    }
  }

  jumanpp::core::features::impl::NgramFeatureData features() {
    auto resSl = slice(&result, NumExamples);
    auto t0SL = slice(&t0, NumExamples);
    util::MutableArraySlice<u64> t1s{&t1};
    util::MutableArraySlice<u64> t2s{&t2};
    return {
        resSl,
        t2s,
        t1s,
        t0SL
    };
  }
};

TEST_CASE("feature values are the same for a small example") {
  TestEnv env;
  env.spec([](ModelSpecBuilder& msb) {
    jumanpp::codegentest::CgTwoSpecFactory::fillSpec(msb);
  });
  env.importDic("a,b,c\nc,d,e\nf,g,h\n");

  JumanppEnv jenv1;
  env.loadEnv(&jenv1);
  jumanpp_generated::Test02 features;
  REQUIRE_OK(jenv1.initFeatures(&features));

  BenchInput b1;
  BenchInput b2;

  auto core = jenv1.coreHolder();
  auto& fs = core->features();

  auto nd1 = b1.features();
  auto nd2 = b2.features();
  fs.ngramDynamic->applyBatch(&nd1);
  fs.ngramStatic->applyBatch(&nd2);

  for (int i = 0; i < b1.result.size(); ++i) {
    CAPTURE(i);
    CHECK(b1.result[i] == b2.result[i]);
  }
}