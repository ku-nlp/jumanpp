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
    result.at(1) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 1ULL, t0[0], t1[1]);
    result.at(2) = jumanpp::util::hashing::hashCtSeq(5575847461935ULL, 2ULL, t0[2], t1[2]);
    result.at(3) = jumanpp::util::hashing::hashCtSeq(89213361200927ULL, 3ULL, t0[2], t1[2], t2[2]);
    result.at(4) = jumanpp::util::hashing::hashCtSeq(89213361200927ULL, 4ULL, t0[2], t1[1], t2[2]);
    result.at(5) = jumanpp::util::hashing::hashCtSeq(89213361200927ULL, 5ULL, t0[0], t1[0], t2[2]);
    result.at(6) = jumanpp::util::hashing::hashCtSeq(89213361200927ULL, 6ULL, t0[1], t1[1], t2[2]);

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
    result.at(1) = seaHashSeq(5575847461935ULL, 1ULL, t0[0], t1[1]);
    result.at(2) = seaHashSeq(5575847461935ULL, 2ULL, t0[2], t1[2]);
    result.at(3) = seaHashSeq(89213361200927ULL, 3ULL, t0[2], t1[2], t2[2]);
    result.at(4) = seaHashSeq(89213361200927ULL, 4ULL, t0[2], t1[1], t2[2]);
    result.at(5) = seaHashSeq(89213361200927ULL, 5ULL, t0[0], t1[0], t2[2]);
    result.at(6) = seaHashSeq(89213361200927ULL, 6ULL, t0[1], t1[1], t2[2]);

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
    result.at(1) = seaHashSeq2(t0[0], t1[1], 5575847461935ULL, 1ULL);
    result.at(2) = seaHashSeq2(t0[2], t1[2], 5575847461935ULL, 2ULL);
    result.at(3) = seaHashSeq2(t0[2], t1[2], t2[2], 89213361200927ULL, 3ULL);
    result.at(4) = seaHashSeq2(t0[2], t1[1], t2[2], 89213361200927ULL, 4ULL);
    result.at(5) = seaHashSeq2(t0[0], t1[0], t2[2], 89213361200927ULL, 5ULL);
    result.at(6) = seaHashSeq2(t0[1], t1[1], t2[2], 89213361200927ULL, 6ULL);

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

BENCHMARK("args first", [](benchpress::context* ctx) {
  std::vector<u32> result(70, 0);
  std::vector<u64> t0(30, 1);
  std::vector<u64> t1(3, 2);
  std::vector<u64> t2(3, 3);
  auto resSl = slice(&result, 10);
  auto t0SL = slice(&t0, 10);
  util::MutableArraySlice<u64> t1s{&t1};
  util::MutableArraySlice<u64> t2s{&t2};
  auto jfd = jumanpp::core::features::impl::NgramFeatureData(
      resSl,
      t2s,
      t1s,
      t0SL
  );
  jumanpp_generated::NgramFeatureStaticApply_Op1_Bench1 b1;
  benchpress::clobber();
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    b1.applyBatch(&jfd);
    //benchpress::clobber();
  }
})

BENCHMARK("args first + seahash", [](benchpress::context* ctx) {
  std::vector<u32> result(70, 0);
  std::vector<u64> t0(30, 1);
  std::vector<u64> t1(3, 2);
  std::vector<u64> t2(3, 3);
  auto resSl = slice(&result, 10);
  auto t0SL = slice(&t0, 10);
  util::MutableArraySlice<u64> t1s{&t1};
  util::MutableArraySlice<u64> t2s{&t2};
  auto jfd = jumanpp::core::features::impl::NgramFeatureData(
      resSl,
      t2s,
      t1s,
      t0SL
  );
  jumanpp_generated::NgramFeatureStaticApply_Op1_Bench1_SH b1;
  benchpress::clobber();
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    b1.applyBatch(&jfd);
    //benchpress::clobber();
  }
})

BENCHMARK("args first + seahash2", [](benchpress::context* ctx) {
  std::vector<u32> result(70, 0);
  std::vector<u64> t0(30, 1);
  std::vector<u64> t1(3, 2);
  std::vector<u64> t2(3, 3);
  auto resSl = slice(&result, 10);
  auto t0SL = slice(&t0, 10);
  util::MutableArraySlice<u64> t1s{&t1};
  util::MutableArraySlice<u64> t2s{&t2};
  auto jfd = jumanpp::core::features::impl::NgramFeatureData(
      resSl,
      t2s,
      t1s,
      t0SL
  );
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

  std::vector<u32> result(70, 0);
  std::vector<u64> t0(30, 1);
  std::vector<u64> t1(3, 2);
  std::vector<u64> t2(3, 3);
  auto resSl = slice(&result, 10);
  auto t0SL = slice(&t0, 10);
  util::MutableArraySlice<u64> t1s{&t1};
  util::MutableArraySlice<u64> t2s{&t2};
  auto jfd = jumanpp::core::features::impl::NgramFeatureData(
      resSl,
      t2s,
      t1s,
      t0SL
  );
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