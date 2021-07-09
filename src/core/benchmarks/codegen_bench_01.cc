//
// Created by Arseny Tolmachev on 2017/05/26.
//

#define BENCHPRESS_CONFIG_MAIN
#include "benchpress/benchpress.hpp"

#include "core/benchmarks/other/spec_01.h"
#include "core/core.h"
#include "core/features_api.h"
#include "core/impl/feature_impl_combine.h"
#include "util/hashing.h"
#include "util/seahash.h"
#include "util/status.hpp"

namespace jumanpp_generated {
class Op1_Bench1 : public jumanpp::core::features::StaticFeatureFactory {
  jumanpp::core::features::NgramFeatureApply *ngram() const override;
};
} // namespace jumanpp_generated

using u64 = jumanpp::u64;
using namespace jumanpp::util::hashing;

namespace jumanpp_generated {
namespace {
class NgramFeatureStaticApply_Op1_Bench1 final
    : public jumanpp::core::features::impl::NgramFeatureApplyImpl<NgramFeatureStaticApply_Op1_Bench1> {
public:
  inline void apply(jumanpp::util::MutableArraySlice<jumanpp::u32> result, const jumanpp::util::ArraySlice<jumanpp::u64> &t2,
                    const jumanpp::util::ArraySlice<jumanpp::u64> &t1, const jumanpp::util::ArraySlice<jumanpp::u64> &t0) const noexcept {
    result.at(0) = jumanpp::util::hashing::hashCtSeq(5575843856927ULL, 0ULL, t0[0]);
    result.at(1) = jumanpp::util::hashing::hashCtSeq(5575843856927ULL, 1ULL, t0[1]);
    result.at(2) = jumanpp::util::hashing::hashCtSeq(5575843856927ULL, 2ULL, t0[2]);
    result.at(3) = jumanpp::util::hashing::hashCtSeq(5575843856927ULL, 3ULL, t0[3]);
    result.at(4) = jumanpp::util::hashing::hashCtSeq(5575843856927ULL, 4ULL, t0[4]);
    result.at(5) = jumanpp::util::hashing::hashCtSeq(5575843856927ULL, 5ULL, t0[5]);
    result.at(6) = jumanpp::util::hashing::hashCtSeq(5575843856927ULL, 6ULL, t0[6]);
    result.at(7) = jumanpp::util::hashing::hashCtSeq(5575843856927ULL, 7ULL, t0[7]);
    result.at(8) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 8ULL, t0[1], t1[0]);
    result.at(9) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 9ULL, t0[8], t1[0]);
    result.at(10) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 10ULL, t0[9], t1[0]);
    result.at(11) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 11ULL, t0[10], t1[0]);
    result.at(12) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 12ULL, t0[11], t1[0]);
    result.at(13) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 13ULL, t0[0], t1[8]);
    result.at(14) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 14ULL, t0[8], t1[8]);
    result.at(15) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 15ULL, t0[9], t1[8]);
    result.at(16) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 16ULL, t0[10], t1[8]);
    result.at(17) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 17ULL, t0[11], t1[8]);
    result.at(18) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 18ULL, t0[0], t1[9]);
    result.at(19) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 19ULL, t0[8], t1[9]);
    result.at(20) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 20ULL, t0[9], t1[9]);
    result.at(21) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 21ULL, t0[10], t1[9]);
    result.at(22) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 22ULL, t0[11], t1[9]);
    result.at(23) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 23ULL, t0[0], t1[10]);
    result.at(24) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 24ULL, t0[8], t1[10]);
    result.at(25) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 25ULL, t0[9], t1[10]);
    result.at(26) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 26ULL, t0[10], t1[10]);
    result.at(27) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 27ULL, t0[11], t1[10]);
    result.at(28) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 28ULL, t0[0], t1[11]);
    result.at(29) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 29ULL, t0[8], t1[11]);
    result.at(30) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 30ULL, t0[9], t1[11]);
    result.at(31) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 31ULL, t0[10], t1[11]);
    result.at(32) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 32ULL, t0[11], t1[11]);
    result.at(33) = jumanpp::util::hashing::hashCtSeq(89213361200927ULL, 33ULL, t0[8], t1[8], t2[8]);
    result.at(34) = jumanpp::util::hashing::hashCtSeq(89213361200927ULL, 34ULL, t0[8], t1[1], t2[8]);
    result.at(35) = jumanpp::util::hashing::hashCtSeq(89213361200927ULL, 35ULL, t0[8], t1[0], t2[0]);
    result.at(36) = jumanpp::util::hashing::hashCtSeq(89213361200927ULL, 36ULL, t0[8], t1[1], t2[1]);
    result.at(37) = jumanpp::util::hashing::hashCtSeq(89213361200927ULL, 37ULL, t0[8], t1[1], t2[12]);
    result.at(38) = jumanpp::util::hashing::hashCtSeq(89213361200927ULL, 38ULL, t0[8], t1[13], t2[12]);
    result.at(39) = jumanpp::util::hashing::hashCtSeq(89213361200927ULL, 39ULL, t0[14], t1[15], t2[12]);

  } // void apply
};  // class NgramFeatureStaticApply_Op1
} // namespace
jumanpp::core::features::NgramFeatureApply *Op1_Bench1::ngram() const { return new NgramFeatureStaticApply_Op1_Bench1{}; }
} // namespace jumanpp_generated

namespace jumanpp_generated {
namespace {
class NgramFeatureStaticApply_Op1_Bench1_SH final
    : public jumanpp::core::features::impl::NgramFeatureApplyImpl<NgramFeatureStaticApply_Op1_Bench1_SH> {
public:
  inline void apply(jumanpp::util::MutableArraySlice<jumanpp::u32> result, const jumanpp::util::ArraySlice<jumanpp::u64> &t2,
                    const jumanpp::util::ArraySlice<jumanpp::u64> &t1, const jumanpp::util::ArraySlice<jumanpp::u64> &t0) const noexcept {
    result.at(0) = seaHashSeq(5575843856927ULL, 0ULL, t0[0]);
    result.at(1) = seaHashSeq(5575843856927ULL, 1ULL, t0[1]);
    result.at(2) = seaHashSeq(5575843856927ULL, 2ULL, t0[2]);
    result.at(3) = seaHashSeq(5575843856927ULL, 3ULL, t0[3]);
    result.at(4) = seaHashSeq(5575843856927ULL, 4ULL, t0[4]);
    result.at(5) = seaHashSeq(5575843856927ULL, 5ULL, t0[5]);
    result.at(6) = seaHashSeq(5575843856927ULL, 6ULL, t0[6]);
    result.at(7) = seaHashSeq(5575843856927ULL, 7ULL, t0[7]);
    result.at(8) = seaHashSeq(5575847461935ULL, 8ULL, t0[1], t1[0]);
    result.at(9) = seaHashSeq(5575847461935ULL, 9ULL, t0[8], t1[0]);
    result.at(10) = seaHashSeq(5575847461935ULL, 10ULL, t0[9], t1[0]);
    result.at(11) = seaHashSeq(5575847461935ULL, 11ULL, t0[10], t1[0]);
    result.at(12) = seaHashSeq(5575847461935ULL, 12ULL, t0[11], t1[0]);
    result.at(13) = seaHashSeq(5575847461935ULL, 13ULL, t0[0], t1[8]);
    result.at(14) = seaHashSeq(5575847461935ULL, 14ULL, t0[8], t1[8]);
    result.at(15) = seaHashSeq(5575847461935ULL, 15ULL, t0[9], t1[8]);
    result.at(16) = seaHashSeq(5575847461935ULL, 16ULL, t0[10], t1[8]);
    result.at(17) = seaHashSeq(5575847461935ULL, 17ULL, t0[11], t1[8]);
    result.at(18) = seaHashSeq(5575847461935ULL, 18ULL, t0[0], t1[9]);
    result.at(19) = seaHashSeq(5575847461935ULL, 19ULL, t0[8], t1[9]);
    result.at(20) = seaHashSeq(5575847461935ULL, 20ULL, t0[9], t1[9]);
    result.at(21) = seaHashSeq(5575847461935ULL, 21ULL, t0[10], t1[9]);
    result.at(22) = seaHashSeq(5575847461935ULL, 22ULL, t0[11], t1[9]);
    result.at(23) = seaHashSeq(5575847461935ULL, 23ULL, t0[0], t1[10]);
    result.at(24) = seaHashSeq(5575847461935ULL, 24ULL, t0[8], t1[10]);
    result.at(25) = seaHashSeq(5575847461935ULL, 25ULL, t0[9], t1[10]);
    result.at(26) = seaHashSeq(5575847461935ULL, 26ULL, t0[10], t1[10]);
    result.at(27) = seaHashSeq(5575847461935ULL, 27ULL, t0[11], t1[10]);
    result.at(28) = seaHashSeq(5575847461935ULL, 28ULL, t0[0], t1[11]);
    result.at(29) = seaHashSeq(5575847461935ULL, 29ULL, t0[8], t1[11]);
    result.at(30) = seaHashSeq(5575847461935ULL, 30ULL, t0[9], t1[11]);
    result.at(31) = seaHashSeq(5575847461935ULL, 31ULL, t0[10], t1[11]);
    result.at(32) = seaHashSeq(5575847461935ULL, 32ULL, t0[11], t1[11]);
    result.at(33) = seaHashSeq(89213361200927ULL, 33ULL, t0[8], t1[8], t2[8]);
    result.at(34) = seaHashSeq(89213361200927ULL, 34ULL, t0[8], t1[1], t2[8]);
    result.at(35) = seaHashSeq(89213361200927ULL, 35ULL, t0[8], t1[0], t2[0]);
    result.at(36) = seaHashSeq(89213361200927ULL, 36ULL, t0[8], t1[1], t2[1]);
    result.at(37) = seaHashSeq(89213361200927ULL, 37ULL, t0[8], t1[1], t2[12]);
    result.at(38) = seaHashSeq(89213361200927ULL, 38ULL, t0[8], t1[13], t2[12]);
    result.at(39) = seaHashSeq(89213361200927ULL, 39ULL, t0[14], t1[15], t2[12]);

  } // void apply
};  // class NgramFeatureStaticApply_Op1
} // namespace
} // namespace jumanpp_generated

namespace jumanpp_generated {
namespace {
class NgramFeatureStaticApply_Op1_Bench1_SH2 final
    : public jumanpp::core::features::impl::NgramFeatureApplyImpl<NgramFeatureStaticApply_Op1_Bench1_SH2> {
public:
  inline void apply(jumanpp::util::MutableArraySlice<jumanpp::u32> result, const jumanpp::util::ArraySlice<jumanpp::u64> &t2,
                    const jumanpp::util::ArraySlice<jumanpp::u64> &t1, const jumanpp::util::ArraySlice<jumanpp::u64> &t0) const noexcept {
    result.at(0) = seaHashSeq2(t0[0], 5575843856927ULL, 0ULL);
    result.at(1) = seaHashSeq2(t0[1], 5575843856927ULL, 1ULL);
    result.at(2) = seaHashSeq2(t0[2], 5575843856927ULL, 2ULL);
    result.at(3) = seaHashSeq2(t0[3], 5575843856927ULL, 3ULL);
    result.at(4) = seaHashSeq2(t0[4], 5575843856927ULL, 4ULL);
    result.at(5) = seaHashSeq2(t0[5], 5575843856927ULL, 5ULL);
    result.at(6) = seaHashSeq2(t0[6], 5575843856927ULL, 6ULL);
    result.at(7) = seaHashSeq2(t0[7], 5575843856927ULL, 7ULL);
    result.at(8) = seaHashSeq2(t0[1], t1[0], 5575847461935ULL, 8ULL);
    result.at(9) = seaHashSeq2(t0[8], t1[0], 5575847461935ULL, 9ULL);
    result.at(10) = seaHashSeq2(t0[9], t1[0], 5575847461935ULL, 10ULL);
    result.at(11) = seaHashSeq2(t0[10], t1[0], 5575847461935ULL, 11ULL);
    result.at(12) = seaHashSeq2(t0[11], t1[0], 5575847461935ULL, 12ULL);
    result.at(13) = seaHashSeq2(t0[0], t1[8], 5575847461935ULL, 13ULL);
    result.at(14) = seaHashSeq2(t0[8], t1[8], 5575847461935ULL, 14ULL);
    result.at(15) = seaHashSeq2(t0[9], t1[8], 5575847461935ULL, 15ULL);
    result.at(16) = seaHashSeq2(t0[10], t1[8], 5575847461935ULL, 16ULL);
    result.at(17) = seaHashSeq2(t0[11], t1[8], 5575847461935ULL, 17ULL);
    result.at(18) = seaHashSeq2(t0[0], t1[9], 5575847461935ULL, 18ULL);
    result.at(19) = seaHashSeq2(t0[8], t1[9], 5575847461935ULL, 19ULL);
    result.at(20) = seaHashSeq2(t0[9], t1[9], 5575847461935ULL, 20ULL);
    result.at(21) = seaHashSeq2(t0[10], t1[9], 5575847461935ULL, 21ULL);
    result.at(22) = seaHashSeq2(t0[11], t1[9], 5575847461935ULL, 22ULL);
    result.at(23) = seaHashSeq2(t0[0], t1[10], 5575847461935ULL, 23ULL);
    result.at(24) = seaHashSeq2(t0[8], t1[10], 5575847461935ULL, 24ULL);
    result.at(25) = seaHashSeq2(t0[9], t1[10], 5575847461935ULL, 25ULL);
    result.at(26) = seaHashSeq2(t0[10], t1[10], 5575847461935ULL, 26ULL);
    result.at(27) = seaHashSeq2(t0[11], t1[10], 5575847461935ULL, 27ULL);
    result.at(28) = seaHashSeq2(t0[0], t1[11], 5575847461935ULL, 28ULL);
    result.at(29) = seaHashSeq2(t0[8], t1[11], 5575847461935ULL, 29ULL);
    result.at(30) = seaHashSeq2(t0[9], t1[11], 5575847461935ULL, 30ULL);
    result.at(31) = seaHashSeq2(t0[10], t1[11], 5575847461935ULL, 31ULL);
    result.at(32) = seaHashSeq2(t0[11], t1[11], 5575847461935ULL, 32ULL);
    result.at(33) = seaHashSeq2(t0[8], t1[8], t2[8], 89213361200927ULL, 33ULL);
    result.at(34) = seaHashSeq2(t0[8], t1[1], t2[8], 89213361200927ULL, 34ULL);
    result.at(35) = seaHashSeq2(t0[8], t1[0], t2[0], 89213361200927ULL, 35ULL);
    result.at(36) = seaHashSeq2(t0[8], t1[1], t2[1], 89213361200927ULL, 36ULL);
    result.at(37) = seaHashSeq2(t0[8], t1[1], t2[12], 89213361200927ULL, 37ULL);
    result.at(38) = seaHashSeq2(t0[8], t1[13], t2[12], 89213361200927ULL, 38ULL);
    result.at(39) = seaHashSeq2(t0[14], t1[15], t2[12], 89213361200927ULL, 39ULL);

  } // void apply
};  // class NgramFeatureStaticApply_Op1
} // namespace
} // namespace jumanpp_generated

namespace jumanpp_generated {
namespace {
class NgramFeatureStaticApply_Baseline final
    : public jumanpp::core::features::impl::NgramFeatureApplyImpl<NgramFeatureStaticApply_Baseline> {
public:
  inline void apply(jumanpp::util::MutableArraySlice<jumanpp::u32> result, const jumanpp::util::ArraySlice<jumanpp::u64> &t2,
                    const jumanpp::util::ArraySlice<jumanpp::u64> &t1, const jumanpp::util::ArraySlice<jumanpp::u64> &t0) const noexcept {
    benchpress::clobber();
  } // void apply
};  // class NgramFeatureStaticApply_Op1
} // namespace
} // namespace jumanpp_generated

using namespace jumanpp;

template <typename T> util::Sliceable<T> slice(std::vector<T> *slc, size_t rows) {
  util::MutableArraySlice<T> mas{slc};
  return util::Sliceable<T>{mas, slc->size() / rows, rows};
}

template <typename T, size_t N> util::Sliceable<T> slice(std::array<T, N> *slc, size_t rows) {
  util::MutableArraySlice<T> mas{slc->data(), N};
  return util::Sliceable<T>{mas, slc->size() / rows, rows};
}

constexpr size_t NumNgrams = 40;
constexpr size_t NumExamples = 20;
constexpr size_t NumFeatures = 16;

struct BenchInput {
  std::array<u32, NumNgrams * NumExamples> result;
  std::array<u64, NumFeatures * NumExamples> t0;
  std::array<u64, NumFeatures> t1;
  std::array<u64, NumFeatures> t2;

  BenchInput() {
    std::fill(result.begin(), result.end(), 0);
    std::fill(t0.begin(), t0.end(), 1);
    std::fill(t1.begin(), t1.end(), 2);
    std::fill(t2.begin(), t2.end(), 3);
  }

  JPP_NO_INLINE jumanpp::core::features::impl::NgramFeatureData features() {
    auto resSl = slice(&result, NumExamples);
    auto t0SL = slice(&t0, NumExamples);
    util::MutableArraySlice<u64> t1s{&t1};
    util::MutableArraySlice<u64> t2s{&t2};
    return {resSl, t2s, t1s, t0SL};
  }
};

template <typename T> JPP_NO_INLINE jumanpp::core::features::NgramFeatureApply *benchRef(T &ref) { return &ref; }

BENCHMARK("args first", [](benchpress::context *ctx) {
  BenchInput inp;
  jumanpp_generated::NgramFeatureStaticApply_Op1_Bench1 b1;
  auto nfa = benchRef(b1);
  benchpress::clobber();
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    auto jfd = inp.features();
    nfa->applyBatch(&jfd);
    benchpress::clobber();
  }
})

void seaHash1(benchpress::context *ctx) {
  BenchInput inp;
  jumanpp_generated::NgramFeatureStaticApply_Op1_Bench1_SH b1;
  auto nfa = benchRef(b1);
  benchpress::clobber();
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    auto jfd = inp.features();
    nfa->applyBatch(&jfd);
    benchpress::clobber();
  }
}

BENCHMARK("args first + seahash", [](benchpress::context *ctx) { seaHash1(ctx); })

BENCHMARK("args first + seahash2", [](benchpress::context *ctx) {
  BenchInput inp;
  jumanpp_generated::NgramFeatureStaticApply_Op1_Bench1_SH2 b1;
  auto nfa = benchRef(b1);
  benchpress::clobber();
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    auto jfd = inp.features();
    nfa->applyBatch(&jfd);
    benchpress::clobber();
  }
})

void doWork2(benchpress::context *ctx) {
  jumanpp::core::spec::dsl::ModelSpecBuilder bldr;
  jumanpp::codegentest::CgOneSpecFactory::fillSpec(bldr);

  jumanpp::core::spec::AnalysisSpec spec;
  auto s = bldr.build(&spec);
  if (!s) {
    std::cerr << "failed to build spec: " << s;
    throw std::exception();
  }

  jumanpp::core::dic::DictionaryBuilder dbld;
  s = dbld.importSpec(&spec);
  s = dbld.importCsv("none", "");
  if (!s) {
    std::cerr << "failed to import empty dic: " << s;
    throw std::exception();
  }

  jumanpp::core::dic::DictionaryHolder dh;
  if (!(s = dh.load(dbld.result()))) {
    std::cerr << "failed to build dicholder " << s;
    throw std::exception();
  }

  jumanpp::core::features::FeatureHolder fh;

  jumanpp::core::CoreHolder core{spec, dh};

  if (!jumanpp::core::features::makeFeatures(core, nullptr, &fh)) {
    throw std::exception();
  }

  BenchInput inp;
  auto nfa = benchRef(*fh.ngram);
  benchpress::clobber();
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    auto jfd = inp.features();
    nfa->applyBatch(&jfd);
    benchpress::clobber();
  }
}

BENCHMARK("dynamic", [](benchpress::context *ctx) { doWork2(ctx); })

BENCHMARK("noop", [](benchpress::context *ctx) {
  BenchInput inp;
  jumanpp_generated::NgramFeatureStaticApply_Baseline b1;
  auto nfa = benchRef(b1);
  benchpress::clobber();
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    auto jfd = inp.features();
    nfa->applyBatch(&jfd);
    benchpress::clobber();
  }
})