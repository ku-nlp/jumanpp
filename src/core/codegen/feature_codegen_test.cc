//
// Created by Arseny Tolmachev on 2017/05/29.
//

#include <array>
#include "cg_2_spec.h"
#include "cgtest02.h"
#include "core/analysis/perceptron.h"
#include "core/impl/feature_impl_combine.h"
#include "core/impl/feature_impl_ngram_partial.h"
#include "testing/test_analyzer.h"

using namespace jumanpp::testing;
using namespace jumanpp::core::spec::dsl;
using namespace jumanpp;

constexpr size_t NumNgrams = 14;
constexpr size_t NumExamples = 10;
constexpr size_t NumFeatures = 5;

template <typename T, size_t N>
util::Sliceable<T> slice(std::array<T, N>* slc, size_t rows) {
  util::MutableArraySlice<T> mas{slc->data(), N};
  return util::Sliceable<T>{mas, slc->size() / rows, rows};
}

struct BenchInput {
  std::array<u32, NumNgrams * NumExamples> result;
  std::array<u64, NumFeatures * NumExamples> pat;
  std::array<u64, NumFeatures * NumExamples> t0;
  std::array<u64, NumFeatures> t1;
  std::array<u64, NumFeatures> t2;

  BenchInput() {
    for (int f = 0; f < NumFeatures; ++f) {
      for (int ex = 0; ex < NumExamples; ++ex) {
        t0.at(f + ex * NumFeatures) = 10000 + 1000 * f + ex * 2;
        pat.at(f + ex * NumFeatures) = 10000 + 1000 * f + ex * 2;
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
    return {resSl, t2s, t1s, t0SL};
  }

  jumanpp::core::features::impl::PatternFeatureData patternData() {
    auto s1 = slice(&pat, NumExamples);
    auto s2 = slice(&t0, NumExamples);
    return {s1, s2};
  }
};

#if 0

TEST_CASE("pattern feature produce the same result") {
  TestEnv env;
  env.spec([](ModelSpecBuilder& msb) {
    jumanpp::codegentest::CgTwoSpecFactory::fillSpec(msb);
  });
  env.importDic("a,b,c\nc,d,e\nf,g,h\n");
  JumanppEnv jenv1;
  env.loadEnv(&jenv1);
  jumanpp_generated::Test02 features;
  REQUIRE_OK(jenv1.initFeatures(&features));
  auto& fs = jenv1.coreHolder()->features();
  BenchInput i1;
  BenchInput i2;

  auto d1 = i1.patternData();
  fs.patternDynamic->applyBatch(&d1);
  auto d2 = i2.patternData();
  fs.patternStatic->applyBatch(&d2);
  for (int i = 0; i < i1.t0.size(); ++i) {
    CAPTURE(i);
    CHECK(i1.t0[i] == i2.t0[i]);
  }
}

#endif

TEST_CASE("full ngram feature values are the same for a small example") {
  TestEnv env;
  env.spec([](ModelSpecBuilder& msb) {
    jumanpp::codegentest::CgTwoSpecFactory::fillSpec(msb);
  });
  env.importDic("a,b,c\nc,d,e\nf,g,h\n");
  REQUIRE(env.core->spec().features.ngram.size() == NumNgrams);

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

TEST_CASE("partial ngram feature computation produces the same values") {
  TestEnv env;
  env.spec([](ModelSpecBuilder& msb) {
    jumanpp::codegentest::CgTwoSpecFactory::fillSpec(msb);
  });
  env.importDic("a,b,c\nc,d,e\nf,g,h\n");

  JumanppEnv jenv1;
  env.loadEnv(&jenv1);
  jumanpp_generated::Test02 features;
  REQUIRE_OK(jenv1.initFeatures(&features));

  auto core = jenv1.coreHolder();
  auto& fs = core->features();

  features::FeatureBuffer fb1;
  features::FeatureBuffer fb2;
  features::AnalysisRunStats ars;
  ars.maxStarts = NumExamples;
  ars.maxEnds = NumExamples;

  util::ArraySlice<float> weights{
      1.0f, 5.0f, 10.0f, 50.0f, 100.0f, 500.0f, 0.1f, 0.5f,
      1.0f, 5.0f, 10.0f, 50.0f, 100.0f, 500.0f, 0.1f, 0.5f,
  };
  core::analysis::HashedFeaturePerceptron perc{weights};

  fs.ngramPartialStatic->allocateBuffers(&fb1, ars, env.analyzer->alloc());
  fs.ngramPartialDynamic->allocateBuffers(&fb2, ars, env.analyzer->alloc());

  BenchInput inp;

  util::ConstSliceable<u64> t0i{inp.t0, NumFeatures, NumExamples};

  float result1[NumExamples];
  float result2[NumExamples];

  fs.ngramPartialDynamic->applyUni(&fb1, t0i, &perc, result1);
  fs.ngramPartialStatic->applyUni(&fb2, t0i, &perc, result2);

  for (int i = 0; i < NumExamples; ++i) {
    CAPTURE(i);
    CHECK(result1[i] == Approx(result2[i]));
  }

  fs.ngramPartialDynamic->applyBiStep1(&fb1, t0i);
  fs.ngramPartialDynamic->applyBiStep2(&fb1, inp.t1, &perc, result1);
  fs.ngramPartialStatic->applyBiStep1(&fb2, t0i);
  fs.ngramPartialStatic->applyBiStep2(&fb2, inp.t1, &perc, result2);

  for (int i = 0; i < NumExamples; ++i) {
    CAPTURE(i);
    CHECK(result1[i] == Approx(result2[i]));
  }

  fs.ngramPartialDynamic->applyTriStep1(&fb1, t0i);
  fs.ngramPartialDynamic->applyTriStep2(&fb1, inp.t1);
  fs.ngramPartialDynamic->applyTriStep3(&fb1, inp.t2, &perc, result1);
  fs.ngramPartialStatic->applyTriStep1(&fb1, t0i);
  fs.ngramPartialStatic->applyTriStep2(&fb1, inp.t1);
  fs.ngramPartialStatic->applyTriStep3(&fb1, inp.t2, &perc, result2);

  for (int i = 0; i < NumExamples; ++i) {
    CAPTURE(i);
    CHECK(result1[i] == Approx(result2[i]));
  }
}

TEST_CASE("partial ngram joint biTri produces the same values") {
  TestEnv env;
  env.spec([](ModelSpecBuilder& msb) {
    jumanpp::codegentest::CgTwoSpecFactory::fillSpec(msb);
  });
  env.importDic("a,b,c\nc,d,e\nf,g,h\n");

  JumanppEnv jenv1;
  env.loadEnv(&jenv1);
  jumanpp_generated::Test02 features;
  REQUIRE_OK(jenv1.initFeatures(&features));

  auto core = jenv1.coreHolder();
  auto& fs = core->features();

  features::FeatureBuffer fb1;
  features::FeatureBuffer fb2;
  features::AnalysisRunStats ars{};
  ars.maxStarts = NumExamples;
  ars.maxEnds = NumExamples;

  float weights[] = {
      1.0f, 5.0f, 10.0f, 50.0f, 100.0f, 500.0f, 0.1f, 0.5f,
      1.0f, 5.0f, 10.0f, 50.0f, 100.0f, 500.0f, 0.1f, 0.5f,
  };
  core::analysis::HashedFeaturePerceptron perc{weights};

  fs.ngramPartialStatic->allocateBuffers(&fb1, ars, env.analyzer->alloc());
  fs.ngramPartialDynamic->allocateBuffers(&fb2, ars, env.analyzer->alloc());

  BenchInput inp;

  float result1[NumExamples];
  float result2[NumExamples];

  util::ConstSliceable<u64> sl0{inp.t0, NumFeatures, NumExamples};
  auto subset = sl0.rows(2, 5);
  u32 idxes[] = {
      0, 2, 1, 2, 1, 0, 0, 1, 0, 0,
  };

  util::fill(fb2.valueBuffer1, 0);
  util::fill(fb2.valueBuffer2, 0);

  util::ConstSliceable<u64> t0fake{inp.t1, NumFeatures, 1};

  fs.ngramPartialStatic->applyBiStep1(&fb2, t0fake);
  fs.ngramPartialDynamic->applyTriStep1(&fb2, t0fake);

  fs.ngramPartialDynamic->applyBiTri(&fb1, 0, inp.t1, subset, sl0, idxes, &perc,
                                     result1);
  fs.ngramPartialStatic->applyBiTri(&fb2, 0, inp.t1, subset, sl0, idxes, &perc,
                                    result2);

  for (int i = 0; i < NumExamples; ++i) {
    CAPTURE(i);
    CHECK(result1[i] == Approx(result2[i]));
  }
}