//
// Created by Arseny Tolmachev on 2017/05/26.
//

#define BENCHPRESS_CONFIG_MAIN
#include "benchpress/benchpress.hpp"

#include "core/features_api.h"
#include "util/hashing.h"
#include "core/impl/feature_impl_combine.h"
#include "cg_1_spec.h"
#include "core/core.h"

namespace jumanpp_generated {
class Op1_Bench1: public jumanpp::core::features::StaticFeatureFactory {
  jumanpp::core::features::NgramFeatureApply* ngram() const override;
};
} //namespace jumanpp_generated


using u64 = jumanpp::u64;


struct SeaHashState {
  u64 s0 = 0x16f11fe89b0d677cULL;
  u64 s1 = 0xb480a793d8e6c86cULL;

  static u64 diffuse(u64 v) {
    v *= 0x6eed0e9da4d94a4fULL;
    auto a = v >> 32;
    auto b = static_cast<unsigned char>(v >> 60);
    v ^= a >> b;
    v *= 0x6eed0e9da4d94a4fULL;
    return v;
  }

  SeaHashState mix(u64 v1, u64 v2) const noexcept {
    return SeaHashState {
        diffuse(s0 ^ v1),
        diffuse(s1 ^ v2)
    };
  }

  SeaHashState mix(u64 v0) {
    return SeaHashState {
        s1,
        diffuse(v0 ^ s0)
    };
  }

  u64 finish() const noexcept {
    return diffuse(s0 ^ s1);
  }
};


void seaHash1(benchpress::context *ctx);

inline SeaHashState seaHashSeqImpl(SeaHashState h) { return h; }

inline SeaHashState seaHashSeqImpl(SeaHashState h, u64 one) { return h.mix(one); }

template <typename... Args>
inline SeaHashState seaHashSeqImpl(SeaHashState h, u64 one, u64 two, Args... args) {
  return seaHashSeqImpl(h.mix(one, two), args...);
}


/**
 * Hash sequence with compile-time passed parameters.
 *
 * Implementation uses C++11 vararg templates.
 * @param seed seed value for hash calcuation
 * @param args values for hash calculation
 * @return hash value
 */
template <typename... Args>
inline u64 seaHashSeq(u64 seed, Args... args) {
  return seaHashSeqImpl(SeaHashState{}, seed, sizeof...(args), static_cast<u64>(args)...)
      .finish();
}


template <typename... Args>
inline u64 seaHashSeq2(Args... args) {
  return seaHashSeqImpl(SeaHashState{}, static_cast<u64>(args)...,
                        sizeof...(args))
      .finish();
}


namespace jumanpp_generated {
namespace {
class NgramFeatureStaticApply_Op1_Bench1 final : public jumanpp::core::features::impl::NgramFeatureApplyImpl< NgramFeatureStaticApply_Op1_Bench1 > {
public:
  inline void apply(jumanpp::util::MutableArraySlice<jumanpp::u32> result,
                    const jumanpp::util::ArraySlice<jumanpp::u64> &t2,
                    const jumanpp::util::ArraySlice<jumanpp::u64> &t1,
                    const jumanpp::util::ArraySlice<jumanpp::u64> &t0) const noexcept {
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
}; // class NgramFeatureStaticApply_Op1
} //anon namespace
jumanpp::core::features::NgramFeatureApply* Op1_Bench1::ngram() const {
  return new NgramFeatureStaticApply_Op1_Bench1{};
}
} //jumanpp_generated namespace


namespace jumanpp_generated {
namespace {
class NgramFeatureStaticApply_Op1_Bench1_SH final : public jumanpp::core::features::impl::NgramFeatureApplyImpl< NgramFeatureStaticApply_Op1_Bench1_SH > {
public:
  inline void apply(jumanpp::util::MutableArraySlice<jumanpp::u32> result,
                    const jumanpp::util::ArraySlice<jumanpp::u64> &t2,
                    const jumanpp::util::ArraySlice<jumanpp::u64> &t1,
                    const jumanpp::util::ArraySlice<jumanpp::u64> &t0) const noexcept {
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
}; // class NgramFeatureStaticApply_Op1
} //anon namespace
} //jumanpp_generated namespace

namespace jumanpp_generated {
namespace {
class NgramFeatureStaticApply_Op1_Bench1_SH2 final : public jumanpp::core::features::impl::NgramFeatureApplyImpl< NgramFeatureStaticApply_Op1_Bench1_SH2 > {
public:
  inline void apply(jumanpp::util::MutableArraySlice<jumanpp::u32> result,
                    const jumanpp::util::ArraySlice<jumanpp::u64> &t2,
                    const jumanpp::util::ArraySlice<jumanpp::u64> &t1,
                    const jumanpp::util::ArraySlice<jumanpp::u64> &t0) const noexcept {
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
}; // class NgramFeatureStaticApply_Op1
} //anon namespace
} //jumanpp_generated namespace

using namespace jumanpp;

template <typename T>
util::Sliceable<T> slice(std::vector<T>* slc, size_t rows) {
  util::MutableArraySlice<T> mas{slc};
  return util::Sliceable<T>{mas, slc->size() / rows, rows};
}

template <typename T, size_t N>
util::Sliceable<T> slice(std::array<T, N>* slc, size_t rows) {
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

  __attribute__((noinline)) jumanpp::core::features::impl::NgramFeatureData features() {
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

BENCHMARK("args first", [](benchpress::context* ctx) {
  BenchInput inp;
  jumanpp_generated::NgramFeatureStaticApply_Op1_Bench1 b1;
  auto jfd = inp.features();
    benchpress::clobber();
    ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    b1.applyBatch(&jfd);
    //benchpress::clobber();
  }
})

BENCHMARK("args first + seahash", [](benchpress::context* ctx) {
  seaHash1(ctx);
})

void seaHash1(benchpress::context *ctx) {
  BenchInput inp;
  auto jfd = inp.features();
  jumanpp_generated::NgramFeatureStaticApply_Op1_Bench1_SH b1;
  benchpress::clobber();
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    b1.applyBatch(&jfd);
    //benchpress::clobber();
  }
}

BENCHMARK("args first + seahash2", [](benchpress::context* ctx) {
  BenchInput inp;
  auto jfd = inp.features();
  jumanpp_generated::NgramFeatureStaticApply_Op1_Bench1_SH2 b1;
  benchpress::clobber();
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    b1.applyBatch(&jfd);
    //benchpress::clobber();
  }
})


void doWork2(benchpress::context* ctx) {
  jumanpp::core::spec::dsl::ModelSpecBuilder bldr;
  jumanpp::codegentest::CgOneSpecFactory::fillSpec(bldr);

  jumanpp::core::spec::AnalysisSpec spec;
  auto s = bldr.build(&spec);
  if (!s) {
    std::cerr << "failed to build spec: " << s.message;
    throw std::exception();
  }

  jumanpp::core::dic::DictionaryBuilder dbld;
  s = dbld.importSpec(&spec);
  s = dbld.importCsv("none", "");
  if (!s) {
    std::cerr << "failed to import empty dic: " << s.message;
    throw std::exception();
  }

  jumanpp::core::dic::DictionaryHolder dh;
  if (!(s = dh.load(dbld.result()))) {
    std::cerr << "failed to build dicholder " << s.message;
    throw std::exception();
  }

  jumanpp::core::RuntimeInfo runtimeInfo;
  if (!dh.compileRuntimeInfo(spec, &runtimeInfo)) {
    throw std::exception();
  }

  jumanpp::core::CoreHolder ch{runtimeInfo, dh};

  jumanpp::core::features::FeatureHolder fh;

  if (!jumanpp::core::features::makeFeatures(ch, nullptr, &fh)) {
    throw std::exception();
  }

  BenchInput inp;
  auto jfd = inp.features();
  auto& b1 = *fh.ngram;
  benchpress::clobber();
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    b1.applyBatch(&jfd);
    //benchpress::clobber();
  }
}

BENCHMARK("dynamic", [](benchpress::context* ctx) {
  doWork2(ctx);
})