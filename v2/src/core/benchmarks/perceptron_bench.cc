//
// Created by Arseny Tolmachev on 2017/10/14.
//

#define BENCHPRESS_CONFIG_MAIN

#include <xmmintrin.h>
#include <random>
#include "benchpress/benchpress.hpp"
#include "util/seahash.h"
#include "util/sliceable_array.h"

using context = benchpress::context;
using namespace jumanpp;

u32 numItems = 50;
u32 numNgrams = 30;
u32 numFeatures = 15;

template <int N, int Size, int Order>
struct FeatureAssignImpl {
  static JPP_ALWAYS_INLINE void assignFeature(
      util::MutableArraySlice<u32> result, util::ArraySlice<u64> state,
      u64 seed);
};

template <int Order, int Size, int N>
struct FieldAssignImpl {
  JPP_ALWAYS_INLINE static void assignFeatureStep(
      util::MutableArraySlice<u32> result, util::ArraySlice<u64> state,
      u64 seed);
};

template <int Size, int N>
struct FieldAssignImpl<3, Size, N> {
  JPP_ALWAYS_INLINE static void assignFeatureStep(
      util::MutableArraySlice<u32> result, util::ArraySlice<u64> state,
      u64 seed) {
    constexpr int theBase = Size - N;
    constexpr u32 baseIdx = theBase * 3;
    // constexpr u32 idx1 = baseIdx + ((N * Size * 512312u) % 15u);
    // constexpr u32 idx2 = baseIdx + ((N * Size * 6712134u) % 15u);
    // constexpr u32 idx3 = baseIdx + ((N * Size * 4854764u) % 15u);
    // result[theBase] = static_cast<u32>(util::hashing::seaHashSeq(state[idx1],
    // state[idx2], state[idx3], seed));
    result[theBase] = static_cast<u32>(
        util::hashing::seaHashSeq(theBase, state[theBase], seed));
  }
};

template <int Size, int Order>
struct FeatureAssignImpl<0, Size, Order> {
  static JPP_ALWAYS_INLINE void assignFeature(
      util::MutableArraySlice<u32> result, util::ArraySlice<u64> state,
      u64 seed) {}
};

template <int N, int Size, int Order>
void FeatureAssignImpl<N, Size, Order>::assignFeature(
    util::MutableArraySlice<u32> result, util::ArraySlice<u64> state,
    u64 seed) {
  FieldAssignImpl<Order, Size, N>::assignFeatureStep(result, state, seed);
  FeatureAssignImpl<N - 1, Size, Order>::assignFeature(result, state, seed);
}

template <int N, int Size, int Order>
struct FeatureAssigner {
  static JPP_ALWAYS_INLINE void assignFeature(
      util::MutableArraySlice<u32> result, util::ArraySlice<u64> state,
      u64 seed) {
    FeatureAssignImpl<N, Size, Order>::assignFeature(result, state, seed);
  }
};

JPP_ALWAYS_INLINE float computingPerceptron30pf1(util::ArraySlice<u64> state,
                                                 u64 seed,
                                                 util::ArraySlice<float> data,
                                                 u32 mask) {
#define index(idx) \
  (mask & static_cast<u32>(util::hashing::seaHashSeq(idx, state[idx], seed)))

  auto base = data.data();

  float t1 = 0;
  float t2 = 0;
  float t3 = 0;
  float t4 = 0;

  u32 i1 = index(0);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i1);
  u32 i2 = index(1);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i2);
  u32 i3 = index(2);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i3);
  u32 i4 = index(3);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i4);
  benchpress::clobber();
  t1 += base[i1];
  t2 += base[i2];
  t3 += base[i3];
  t4 += base[i4];

  i1 = index(4);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i1);
  i2 = index(5);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i2);
  i3 = index(6);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i3);
  i4 = index(7);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i4);
  benchpress::clobber();
  t1 += base[i1];
  t2 += base[i2];
  t3 += base[i3];
  t4 += base[i4];

  i1 = index(8);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i1);
  i2 = index(9);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i2);
  i3 = index(10);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i3);
  i4 = index(11);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i4);
  benchpress::clobber();
  t1 += base[i1];
  t2 += base[i2];
  t3 += base[i3];
  t4 += base[i4];

  i1 = index(12);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i1);
  i2 = index(13);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i2);
  i3 = index(14);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i3);
  i4 = index(15);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i4);
  benchpress::clobber();
  t1 += base[i1];
  t2 += base[i2];
  t3 += base[i3];
  t4 += base[i4];

  i1 = index(16);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i1);
  i2 = index(17);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i2);
  i3 = index(18);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i3);
  i4 = index(19);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i4);
  benchpress::clobber();
  t1 += base[i1];
  t2 += base[i2];
  t3 += base[i3];
  t4 += base[i4];

  i1 = index(20);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i1);
  i2 = index(21);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i2);
  i3 = index(22);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i3);
  i4 = index(23);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i4);
  benchpress::clobber();
  t1 += base[i1];
  t2 += base[i2];
  t3 += base[i3];
  t4 += base[i4];

  i1 = index(24);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i1);
  i2 = index(25);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i2);
  i3 = index(26);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i3);
  i4 = index(27);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i4);
  benchpress::clobber();
  t1 += base[i1];
  t2 += base[i2];
  t3 += base[i3];
  t4 += base[i4];

  i1 = index(28);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i1);
  i2 = index(29);
  util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(base + i2);
  benchpress::clobber();
  t1 += base[i1];
  t2 += base[i2];

#undef index

  return t1 + t2 + t3 + t4;
}

struct InputData {
  std::vector<float> weights;
  std::vector<u64> state;
  std::vector<u32> features;

  InputData() {
    u64 numWeights = 4 * 1024 * 1024;

    std::minstd_rand rng{1};
    std::uniform_real_distribution<float> floats{-0.1f, 0.1f};
    weights.reserve(numWeights);
    for (auto i = 0; i < numWeights; ++i) {
      weights.push_back(floats(rng));
    }
    std::uniform_int_distribution<u64> states{};
    u32 totalStates = numItems * numFeatures;
    state.reserve(totalStates);
    for (int i = 0; i < totalStates; ++i) {
      state.push_back(states(rng));
    }
  }

  void __attribute__((noinline))
  computeFeature(util::MutableArraySlice<u32> result, i32 item, u64 seed) {
    util::ConstSliceable<u64> stateSlice{state, numItems, numFeatures};
    FeatureAssigner<30, 30, 3>::assignFeature(result, stateSlice.row(item),
                                              seed);
  }
} inputs;

template <int Unroll = 1>
struct HorizontalPerceptron {
  static float compute(util::ArraySlice<u32> idxes, u32 mask) {
    util::ArraySlice<float> weights = inputs.weights;
    float result = 0;
    for (int i = 0; i < idxes.size(); ++i) {
      result += weights[idxes[i] & mask];
    }
    return result;
  }
};

template <>
struct HorizontalPerceptron<4> {
  static float compute(util::ArraySlice<u32> idxes, u32 mask) {
    util::ArraySlice<float> weights = inputs.weights;
    constexpr int Step = 4;
    float r1 = 0;
    float r2 = 0;
    float r3 = 0;
    float r4 = 0;
    int i = 0;
    for (; i < idxes.size(); i += Step) {
      r1 += weights[idxes[i + 0] & mask];
      r2 += weights[idxes[i + 1] & mask];
      r3 += weights[idxes[i + 2] & mask];
      r4 += weights[idxes[i + 3] & mask];
    }
    if (i > idxes.size()) {
      i -= 2;
      switch (i & (Step - 1)) {
        case 3:
          r3 += weights[idxes[i + 2] & mask];
        case 2:
          r2 += weights[idxes[i + 1] & mask];
        case 1:
          r1 += weights[idxes[i + 0] & mask];
        default:;
      }
    }

    return r1 + r2 + r3 + r4;
  }
};

template <>
struct HorizontalPerceptron<4000> {
  static float compute(util::ArraySlice<u32> idxes, u32 mask) {
    util::ArraySlice<float> weights = inputs.weights;
    constexpr int Step = 4;
    float r1 = 0;
    float r2 = 0;
    float r3 = 0;
    float r4 = 0;
    float t1 = 0;
    float t2 = 0;
    float t3 = 0;
    float t4 = 0;

    int i = 0;
    for (; i < idxes.size(); i += Step) {
      r1 += t1;
      r2 += t2;
      r3 += t3;
      r4 += t4;
      t1 = weights[idxes[i + 0] & mask];
      t2 = weights[idxes[i + 1] & mask];
      t3 = weights[idxes[i + 2] & mask];
      t4 = weights[idxes[i + 3] & mask];
    }
    if (i > idxes.size()) {
      i -= 2;
      switch (i & (Step - 1)) {
        case 3:
          r3 += weights[idxes[i + 2] & mask];
        case 2:
          r2 += weights[idxes[i + 1] & mask];
        case 1:
          r1 += weights[idxes[i + 0] & mask];
        default:;
      }
    }

    r1 += t1;
    r2 += t2;
    r3 += t3;
    r4 += t4;

    return r1 + r2 + r3 + r4;
  }
};

template <>
struct HorizontalPerceptron<401> {
  static float compute(util::ArraySlice<u32> idxes, u32 mask) {
    util::ArraySlice<float> weights = inputs.weights;
    float* base = const_cast<float*>(weights.data());
    constexpr int Step = 4;
    __m128 r1;
    r1 = _mm_xor_ps(r1, r1);
    __m128 r2 = r1;
    __m128 r3 = r1;
    __m128 r4 = r1;
    __m128 t1 = r1;
    __m128 t2 = r1;
    __m128 t3 = r1;
    __m128 t4 = r1;
    int i = 0;
    auto sz = idxes.size();
    for (; (i + Step) < sz; i += Step) {
      r1[0] += t1[0];
      r2[0] += t2[0];
      r3[0] += t3[0];
      r4[0] += t4[0];
      t1 = _mm_load_ss(base + (idxes[i + 0] & mask));
      t2 = _mm_load_ss(base + (idxes[i + 1] & mask));
      t3 = _mm_load_ss(base + (idxes[i + 2] & mask));
      t4 = _mm_load_ss(base + (idxes[i + 3] & mask));
    }
    switch ((sz - i) & (Step - 1)) {
      case 3:
        r3[0] += _mm_load_ss(base + (idxes[i + 2] & mask))[0];
      case 2:
        r2[0] += _mm_load_ss(base + (idxes[i + 1] & mask))[0];
      case 1:
        r1[0] += _mm_load_ss(base + (idxes[i + 0] & mask))[0];
      default:;
    }

    r1[0] += r2[0] + t2[0];
    r3[0] += r4[0] + t4[0];
    r1[0] += r3[0] + t3[0];

    return r1[0] + t1[0];
  }
};

template <>
struct HorizontalPerceptron<402> {
  static float compute(util::ArraySlice<u32> idxes, u32 mask) {
    util::ArraySlice<float> weights = inputs.weights;
    float* base = const_cast<float*>(weights.data());
    constexpr int Step = 4;
    __m128 r1;
    r1 = _mm_xor_ps(r1, r1);
    __m128 r2 = r1;
    __m128 r3 = r1;
    __m128 r4 = r1;
    __m128 t1 = r1;
    int i = 0;
    auto sz = idxes.size();
    for (; (i + Step) < sz; i += Step) {
      r1 += t1;
      t1[0] = *(base + (idxes[i + 0] & mask));
      t1[1] = *(base + (idxes[i + 1] & mask));
      t1[2] = *(base + (idxes[i + 2] & mask));
      t1[3] = *(base + (idxes[i + 3] & mask));
    }
    switch ((sz - i) & (Step - 1)) {
      case 3:
        r4 = _mm_load_ss(base + (idxes[i + 2] & mask));
      case 2:
        r3 = _mm_load_ss(base + (idxes[i + 1] & mask));
      case 1:
        r2 = _mm_load_ss(base + (idxes[i + 0] & mask));
      default:;
    }

    r2[0] += r3[0] + r4[0];
    r1 += t1;

    return r1[0] + r1[1] + r1[2] + r1[3] + r2[0];
  }
};

template <int N>
static void __attribute__((noinline))
horizontalPerc(util::MutableArraySlice<float> result,
               util::ConstSliceable<u32> features, u32 max) {
  u32 mask = max - 1;
  for (int i = 0; i < features.numRows(); ++i) {
    result[i] += HorizontalPerceptron<N>::compute(features.row(i), mask);
  }
}

struct OutputData {
  std::vector<u32> outputFeatures;
  std::vector<float> outputScores;
  util::Sliceable<u32> features{&outputFeatures, numNgrams, numItems};

  OutputData() : outputFeatures(numItems * numNgrams), outputScores(numItems) {}

  void computeFeatures(u64 seed) {
    for (int i = 0; i < features.numRows(); ++i) {
      inputs.computeFeature(features.row(i), i, seed);
    }
  }

  void __attribute__((noinline)) computeMixedScoresPf1(u64 seed, u32 mask) {
    auto& weights = inputs.weights;
    util::ConstSliceable<u64> stateSlice{inputs.state, numItems, numFeatures};
    for (int i = 0; i < features.numRows(); ++i) {
      outputScores[i] +=
          computingPerceptron30pf1(stateSlice.row(i), seed, weights, mask);
    }
  }
};

volatile u64 size4m = 4 * 1024 * 1024;

BENCHMARK("hor-4-4m", [](context* ctx) {
  OutputData out;
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeFeatures(iter);
    horizontalPerc<4>(&out.outputScores, out.features, size4m);
  }
});

BENCHMARK("hor-4c-4m", [](context* ctx) {
  OutputData out;
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeFeatures(iter);
    horizontalPerc<401>(&out.outputScores, out.features, size4m);
  }
});

BENCHMARK("hor-4d-4m", [](context* ctx) {
  OutputData out;
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeFeatures(iter);
    horizontalPerc<402>(&out.outputScores, out.features, size4m);
  }
});

BENCHMARK("hor-4f-4m", [](context* ctx) {
  OutputData out;
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeFeatures(iter);
    horizontalPerc<4000>(&out.outputScores, out.features, size4m);
  }
});

BENCHMARK("hor-fu-pf1-4m", [](context* ctx) {
  OutputData out;
  u32 mask = static_cast<u32>(size4m - 1);
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeMixedScoresPf1(iter, mask);
  }
});