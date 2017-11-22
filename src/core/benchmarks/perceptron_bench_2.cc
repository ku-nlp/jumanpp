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

float computingPerceptron30(util::ArraySlice<u64> state, u64 seed,
                            util::ArraySlice<float> data, u32 mask) {
  float t1 = 0;
  float t2 = 0;
  float t3 = 0;
  float t4 = 0;
  t1 += data[mask & util::hashing::seaHashSeq(0, state[0], seed)];
  t2 += data[mask & util::hashing::seaHashSeq(1, state[1], seed)];
  t3 += data[mask & util::hashing::seaHashSeq(2, state[2], seed)];
  t4 += data[mask & util::hashing::seaHashSeq(3, state[3], seed)];
  t1 += data[mask & util::hashing::seaHashSeq(4, state[4], seed)];
  t2 += data[mask & util::hashing::seaHashSeq(5, state[5], seed)];
  t3 += data[mask & util::hashing::seaHashSeq(6, state[6], seed)];
  t4 += data[mask & util::hashing::seaHashSeq(7, state[7], seed)];
  t1 += data[mask & util::hashing::seaHashSeq(8, state[8], seed)];
  t2 += data[mask & util::hashing::seaHashSeq(9, state[9], seed)];
  t3 += data[mask & util::hashing::seaHashSeq(10, state[10], seed)];
  t4 += data[mask & util::hashing::seaHashSeq(11, state[11], seed)];
  t1 += data[mask & util::hashing::seaHashSeq(12, state[12], seed)];
  t2 += data[mask & util::hashing::seaHashSeq(13, state[13], seed)];
  t3 += data[mask & util::hashing::seaHashSeq(14, state[14], seed)];
  t4 += data[mask & util::hashing::seaHashSeq(15, state[15], seed)];
  t1 += data[mask & util::hashing::seaHashSeq(16, state[16], seed)];
  t2 += data[mask & util::hashing::seaHashSeq(17, state[17], seed)];
  t3 += data[mask & util::hashing::seaHashSeq(18, state[18], seed)];
  t4 += data[mask & util::hashing::seaHashSeq(19, state[19], seed)];
  t1 += data[mask & util::hashing::seaHashSeq(20, state[20], seed)];
  t2 += data[mask & util::hashing::seaHashSeq(21, state[21], seed)];
  t3 += data[mask & util::hashing::seaHashSeq(22, state[22], seed)];
  t4 += data[mask & util::hashing::seaHashSeq(23, state[23], seed)];
  t1 += data[mask & util::hashing::seaHashSeq(24, state[24], seed)];
  t2 += data[mask & util::hashing::seaHashSeq(25, state[25], seed)];
  t3 += data[mask & util::hashing::seaHashSeq(26, state[26], seed)];
  t4 += data[mask & util::hashing::seaHashSeq(27, state[27], seed)];
  t1 += data[mask & util::hashing::seaHashSeq(28, state[28], seed)];
  t2 += data[mask & util::hashing::seaHashSeq(29, state[29], seed)];
  return t1 + t2 + t3 + t4;
}

float computingPerceptron30a(util::ArraySlice<u64> state, u64 seed,
                             util::ArraySlice<float> data, u32 mask) {
  float t1 = 0;
  float t2 = 0;
  float t3 = 0;
  float t4 = 0;
  float r1 = 0;
  float r2 = 0;
  float r3 = 0;
  float r4 = 0;
  r1 = data[mask & util::hashing::seaHashSeq(0, state[0], seed)];
  r2 = data[mask & util::hashing::seaHashSeq(1, state[1], seed)];
  r3 = data[mask & util::hashing::seaHashSeq(2, state[2], seed)];
  r4 = data[mask & util::hashing::seaHashSeq(3, state[3], seed)];
  t1 += r1;
  t2 += r2;
  t3 += r3;
  t4 += r4;
  r1 = data[mask & util::hashing::seaHashSeq(4, state[4], seed)];
  r2 = data[mask & util::hashing::seaHashSeq(5, state[5], seed)];
  r3 = data[mask & util::hashing::seaHashSeq(6, state[6], seed)];
  r4 = data[mask & util::hashing::seaHashSeq(7, state[7], seed)];
  t1 += r1;
  t2 += r2;
  t3 += r3;
  t4 += r4;
  r1 = data[mask & util::hashing::seaHashSeq(8, state[8], seed)];
  r2 = data[mask & util::hashing::seaHashSeq(9, state[9], seed)];
  r3 = data[mask & util::hashing::seaHashSeq(10, state[10], seed)];
  r4 = data[mask & util::hashing::seaHashSeq(11, state[11], seed)];
  t1 += r1;
  t2 += r2;
  t3 += r3;
  t4 += r4;
  r1 = data[mask & util::hashing::seaHashSeq(12, state[12], seed)];
  r2 = data[mask & util::hashing::seaHashSeq(13, state[13], seed)];
  r3 = data[mask & util::hashing::seaHashSeq(14, state[14], seed)];
  r4 = data[mask & util::hashing::seaHashSeq(15, state[15], seed)];
  t1 += r1;
  t2 += r2;
  t3 += r3;
  t4 += r4;
  r1 = data[mask & util::hashing::seaHashSeq(16, state[16], seed)];
  r2 = data[mask & util::hashing::seaHashSeq(17, state[17], seed)];
  r4 = data[mask & util::hashing::seaHashSeq(19, state[19], seed)];
  r3 = data[mask & util::hashing::seaHashSeq(18, state[18], seed)];
  t1 += r1;
  t2 += r2;
  t3 += r3;
  t4 += r4;
  r1 = data[mask & util::hashing::seaHashSeq(20, state[20], seed)];
  r2 = data[mask & util::hashing::seaHashSeq(21, state[21], seed)];
  r3 = data[mask & util::hashing::seaHashSeq(22, state[22], seed)];
  r4 = data[mask & util::hashing::seaHashSeq(23, state[23], seed)];
  t1 += r1;
  t2 += r2;
  t3 += r3;
  t4 += r4;
  r1 = data[mask & util::hashing::seaHashSeq(24, state[24], seed)];
  r2 = data[mask & util::hashing::seaHashSeq(25, state[25], seed)];
  r3 = data[mask & util::hashing::seaHashSeq(26, state[26], seed)];
  r4 = data[mask & util::hashing::seaHashSeq(27, state[27], seed)];
  t1 += r1;
  t2 += r2;
  t3 += r3;
  t4 += r4;
  r1 = data[mask & util::hashing::seaHashSeq(28, state[28], seed)];
  r2 = data[mask & util::hashing::seaHashSeq(29, state[29], seed)];
  t1 += r1;
  t2 += r2;
  return t1 + t2 + t3 + t4;
}

JPP_ALWAYS_INLINE inline float computingPerceptron30b(
    util::ArraySlice<u64> state, u64 seed, util::ArraySlice<float> datax,
    u32 mask) {
  float* data = const_cast<float*>(datax.data());

  __m128 r1 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(0, state[0], seed)));
  __m128 r2 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(1, state[1], seed)));
  __m128 r3 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(2, state[2], seed)));
  __m128 r4 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(3, state[3], seed)));
  __m128 t1 = r1;
  __m128 t2 = r2;
  __m128 t3 = r3;
  __m128 t4 = r4;
  r1 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(4, state[4], seed)));
  r2 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(5, state[5], seed)));
  r3 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(6, state[6], seed)));
  r4 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(7, state[7], seed)));
  __m128 z1 = r1;
  __m128 z2 = r2;
  __m128 z3 = r3;
  __m128 z4 = r4;
  r1 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(8, state[8], seed)));
  r2 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(9, state[9], seed)));
  r3 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(10, state[10], seed)));
  r4 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(11, state[11], seed)));
  t1[0] += r1[0];
  t2[0] += r2[0];
  t3[0] += r3[0];
  t4[0] += r4[0];
  r1 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(12, state[12], seed)));
  r2 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(13, state[13], seed)));
  r3 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(14, state[14], seed)));
  r4 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(15, state[15], seed)));
  z1[0] += r1[0];
  z2[0] += r2[0];
  z3[0] += r3[0];
  z4[0] += r4[0];
  r1 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(16, state[16], seed)));
  r2 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(17, state[17], seed)));
  r4 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(18, state[18], seed)));
  r3 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(19, state[19], seed)));
  t1[0] += r1[0];
  t2[0] += r2[0];
  t3[0] += r3[0];
  t4[0] += r4[0];
  r1 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(20, state[20], seed)));
  r2 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(21, state[21], seed)));
  r3 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(22, state[22], seed)));
  r4 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(23, state[23], seed)));
  z1[0] += r1[0];
  z2[0] += r2[0];
  z3[0] += r3[0];
  z4[0] += r4[0];
  r1 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(24, state[24], seed)));
  r2 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(25, state[25], seed)));
  r3 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(26, state[26], seed)));
  r4 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(27, state[27], seed)));
  t1[0] += r1[0];
  t2[0] += r2[0];
  t3[0] += r3[0];
  t4[0] += r4[0];
  r1 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(28, state[28], seed)));
  r2 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(29, state[29], seed)));
  z1[0] += r1[0];
  z2[0] += r2[0];
  z3[0] += t3[0];
  z4[0] += t4[0];
  z1[0] += t1[0];
  z2[0] += t2[0];
  return z1[0] + z2[0] + z3[0] + z4[0];
}

JPP_ALWAYS_INLINE inline float computingPerceptron30c(
    util::ArraySlice<u64> state, u64 seed, util::ArraySlice<float> datax,
    u32 mask) {
  float* data = const_cast<float*>(datax.data());

  __m128 r1 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(0, state[0], seed)));
  __m128 r2 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(1, state[1], seed)));
  __m128 r3 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(2, state[2], seed)));
  __m128 r4 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(3, state[3], seed)));
  __m128 d = r1;
  d[1] = r2[0];
  d[2] = r3[0];
  d[3] = r4[0];
  r1 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(4, state[4], seed)));
  r2 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(5, state[5], seed)));
  r3 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(6, state[6], seed)));
  r4 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(7, state[7], seed)));
  __m128 z = r1;
  z[1] = r2[0];
  z[2] = r3[0];
  z[3] = r3[0];
  r1 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(8, state[8], seed)));
  r2 =
      _mm_load_ss(data + (mask & util::hashing::seaHashSeq(9, state[9], seed)));
  r3 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(10, state[10], seed)));
  r4 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(11, state[11], seed)));
  __m128 t1 = d;
  d = r1;
  d[1] = r2[0];
  d[2] = r3[0];
  d[3] = r4[0];
  r1 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(12, state[12], seed)));
  r2 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(13, state[13], seed)));
  r3 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(14, state[14], seed)));
  r4 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(15, state[15], seed)));
  __m128 t2 = z;
  z = r1;
  z[1] = r2[0];
  z[2] = r3[0];
  z[3] = r3[0];
  r1 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(16, state[16], seed)));
  r2 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(17, state[17], seed)));
  r4 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(18, state[18], seed)));
  r3 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(19, state[19], seed)));
  t1 += d;
  d = r1;
  d[1] = r2[0];
  d[2] = r3[0];
  d[3] = r4[0];
  r1 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(20, state[20], seed)));
  r2 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(21, state[21], seed)));
  r3 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(22, state[22], seed)));
  r4 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(23, state[23], seed)));
  t2 += z;
  z = r1;
  z[1] = r2[0];
  z[2] = r3[0];
  z[3] = r3[0];
  r1 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(24, state[24], seed)));
  r2 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(25, state[25], seed)));
  r3 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(26, state[26], seed)));
  r4 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(27, state[27], seed)));
  t1 += d;
  d = r1;
  d[1] = r2[0];
  d[2] = r3[0];
  d[3] = r4[0];
  r1 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(28, state[28], seed)));
  r2 = _mm_load_ss(data +
                   (mask & util::hashing::seaHashSeq(29, state[29], seed)));
  t2 += z;
  z = r1;
  z[1] = r2[0];
  z[2] = 0.0f;
  z[3] = 0.0f;
  t1 += d;
  t2 += z;
  t1 += t2;
  return t1[0] + t1[1] + t1[2] + t1[3];
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
struct HorizontalPerceptron<2> {
  static float compute(util::ArraySlice<u32> idxes, u32 mask) {
    util::ArraySlice<float> weights = inputs.weights;
    constexpr int Step = 2;
    float r1 = 0;
    float r2 = 0;
    int i = 0;
    for (; i < idxes.size(); i += Step) {
      r1 += weights[idxes[i + 0] & mask];
      r2 += weights[idxes[i + 1] & mask];
    }
    if (i > idxes.size()) {
      i -= 2;
      switch (i & (Step - 1)) {
        case 1:
          r1 += weights[idxes[i + 0] & mask];
        default:;
      }
    }

    return r1 + r2;
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
struct HorizontalPerceptron<40> {
  static float compute(util::ArraySlice<u32> idxes, u32 mask) {
    util::ArraySlice<float> weights = inputs.weights;
    constexpr int Step = 4;
    float r1 = 0;
    float r2 = 0;
    float r3 = 0;
    float r4 = 0;
    int i = 0;
    auto sz = idxes.size();
    for (; (i + Step) < sz; i += Step) {
      r1 += weights[idxes[i + 0] & mask];
      r2 += weights[idxes[i + 1] & mask];
      r3 += weights[idxes[i + 2] & mask];
      r4 += weights[idxes[i + 3] & mask];
    }
    switch ((sz - i) & (Step - 1)) {
      case 3:
        r3 += weights[idxes[i + 2] & mask];
      case 2:
        r2 += weights[idxes[i + 1] & mask];
      case 1:
        r1 += weights[idxes[i + 0] & mask];
      default:;
    }

    return r1 + r2 + r3 + r4;
  }
};

template <>
struct HorizontalPerceptron<400> {
  static float compute(util::ArraySlice<u32> idxes, u32 mask) {
    util::ArraySlice<float> weights = inputs.weights;
    float* base = const_cast<float*>(weights.data());
    constexpr int Step = 4;
    __m128 r1;
    r1 = _mm_xor_ps(r1, r1);
    __m128 r2 = r1;
    __m128 r3 = r1;
    __m128 r4 = r1;
    int i = 0;
    auto sz = idxes.size();
    for (; (i + Step) < sz; i += Step) {
      __m128 t1 = _mm_load_ss(base + ((i + 0) & mask));
      __m128 t2 = _mm_load_ss(base + ((i + 1) & mask));
      __m128 t3 = _mm_load_ss(base + ((i + 2) & mask));
      __m128 t4 = _mm_load_ss(base + ((i + 3) & mask));
      r1[0] += t1[0];
      r2[0] += t2[0];
      r3[0] += t3[0];
      r4[0] += t4[0];
    }
    switch ((sz - i) & (Step - 1)) {
      case 3:
        r3[0] += weights[idxes[i + 2] & mask];
      case 2:
        r2[0] += weights[idxes[i + 1] & mask];
      case 1:
        r1[0] += weights[idxes[i + 0] & mask];
      default:;
    }

    r1[0] += r2[0];
    r3[0] += r4[0];
    r1[0] += r3[0];

    return r1[0];
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
      t1 = _mm_load_ss(base + ((i + 0) & mask));
      t2 = _mm_load_ss(base + ((i + 1) & mask));
      t3 = _mm_load_ss(base + ((i + 2) & mask));
      t4 = _mm_load_ss(base + ((i + 3) & mask));
    }
    switch ((sz - i) & (Step - 1)) {
      case 3:
        r3 += _mm_load_ss(base + ((i + 2) & mask));
      case 2:
        r2 += _mm_load_ss(base + ((i + 1) & mask));
      case 1:
        r1 += _mm_load_ss(base + ((i + 0) & mask));
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
      t1[0] = *(base + ((i + 0) & mask));
      t1[1] = *(base + ((i + 1) & mask));
      t1[2] = *(base + ((i + 2) & mask));
      t1[3] = *(base + ((i + 3) & mask));
    }
    switch ((sz - i) & (Step - 1)) {
      case 3:
        r4 = _mm_load_ss(base + ((i + 2) & mask));
      case 2:
        r3 = _mm_load_ss(base + ((i + 1) & mask));
      case 1:
        r2 = _mm_load_ss(base + ((i + 0) & mask));
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

  void __attribute__((noinline)) computeMixedScores(u64 seed, u32 mask) {
    auto& weights = inputs.weights;
    util::ConstSliceable<u64> stateSlice{inputs.state, numItems, numFeatures};
    for (int i = 0; i < features.numRows(); ++i) {
      outputScores[i] +=
          computingPerceptron30(stateSlice.row(i), seed, weights, mask);
    }
  }

  void __attribute__((noinline)) computeMixedScores2(u64 seed, u32 mask) {
    auto& weights = inputs.weights;
    util::ConstSliceable<u64> stateSlice{inputs.state, numItems, numFeatures};
    for (int i = 0; i < features.numRows(); ++i) {
      outputScores[i] +=
          computingPerceptron30a(stateSlice.row(i), seed, weights, mask);
    }
  }

  void __attribute__((noinline)) computeMixedScores3(u64 seed, u32 mask) {
    auto& weights = inputs.weights;
    util::ConstSliceable<u64> stateSlice{inputs.state, numItems, numFeatures};
    for (int i = 0; i < features.numRows(); ++i) {
      outputScores[i] +=
          computingPerceptron30b(stateSlice.row(i), seed, weights, mask);
    }
  }

  void __attribute__((noinline)) computeMixedScores4(u64 seed, u32 mask) {
    auto& weights = inputs.weights;
    util::ConstSliceable<u64> stateSlice{inputs.state, numItems, numFeatures};
    for (int i = 0; i < features.numRows(); ++i) {
      outputScores[i] +=
          computingPerceptron30c(stateSlice.row(i), seed, weights, mask);
    }
  }
};

volatile u64 size4m = 4 * 1024 * 1024;

BENCHMARK("hor-1-4m", [](context* ctx) {
  OutputData out;
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeFeatures(iter);
    horizontalPerc<1>(&out.outputScores, out.features, size4m);
  }
});

BENCHMARK("hor-2-4m", [](context* ctx) {
  OutputData out;
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeFeatures(iter);
    horizontalPerc<2>(&out.outputScores, out.features, size4m);
  }
});

BENCHMARK("hor-4-4m", [](context* ctx) {
  OutputData out;
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeFeatures(iter);
    horizontalPerc<4>(&out.outputScores, out.features, size4m);
  }
});

BENCHMARK("hor-4a-4m", [](context* ctx) {
  OutputData out;
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeFeatures(iter);
    horizontalPerc<40>(&out.outputScores, out.features, size4m);
  }
});

BENCHMARK("hor-4b-4m", [](context* ctx) {
  OutputData out;
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeFeatures(iter);
    horizontalPerc<400>(&out.outputScores, out.features, size4m);
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

BENCHMARK("hor-mix-full-4m", [](context* ctx) {
  OutputData out;
  u32 mask = static_cast<u32>(size4m - 1);
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeMixedScores(iter, mask);
  }
});

BENCHMARK("hor-mix-fulla-4m", [](context* ctx) {
  OutputData out;
  u32 mask = static_cast<u32>(size4m - 1);
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeMixedScores2(iter, mask);
  }
});

BENCHMARK("hor-mix-fullb-4m", [](context* ctx) {
  OutputData out;
  u32 mask = static_cast<u32>(size4m - 1);
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeMixedScores3(iter, mask);
  }
});

BENCHMARK("hor-mix-fullc-4m", [](context* ctx) {
  OutputData out;
  u32 mask = static_cast<u32>(size4m - 1);
  for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
    out.computeMixedScores4(iter, mask);
  }
});