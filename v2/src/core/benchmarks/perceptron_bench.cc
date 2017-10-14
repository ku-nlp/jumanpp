//
// Created by Arseny Tolmachev on 2017/10/14.
//

#define BENCHPRESS_CONFIG_MAIN

#include "benchpress/benchpress.hpp"
#include "util/seahash.h"
#include <random>
#include "util/sliceable_array.h"

using context = benchpress::context;
using namespace jumanpp;

u32 numItems = 50;
u32 numNgrams = 30;
u32 numFeatures = 15;

template <int N, int Size, int Order>
struct FeatureAssignImpl {
    static JPP_ALWAYS_INLINE void assignFeature(util::MutableArraySlice<u32> result, util::ArraySlice<u64> state, u64 seed);
};

template <int Order, int Size, int N>
struct FieldAssignImpl {
    JPP_ALWAYS_INLINE static void assignFeatureStep(util::MutableArraySlice<u32> result, util::ArraySlice<u64> state, u64 seed);
};

template <int Size, int N>
struct FieldAssignImpl<3, Size, N> {
    JPP_ALWAYS_INLINE static void assignFeatureStep(util::MutableArraySlice<u32> result, util::ArraySlice<u64> state, u64 seed) {
      constexpr int theBase = Size - N;
      constexpr u32 baseIdx = theBase * 3;
      constexpr u32 idx1 = baseIdx + ((N * Size * 512312u) % 15u);
      constexpr u32 idx2 = baseIdx + ((N * Size * 6712134u) % 15u);
      constexpr u32 idx3 = baseIdx + ((N * Size * 4854764u) % 15u);
      //result[theBase] = static_cast<u32>(util::hashing::seaHashSeq(state[idx1], state[idx2], state[idx3], seed));
      result[theBase] = static_cast<u32>(util::hashing::seaHashSeq(state[idx1], seed));
    }
};

template <int Size, int Order>
struct FeatureAssignImpl<0, Size, Order> {
    static JPP_ALWAYS_INLINE void assignFeature(util::MutableArraySlice<u32> result, util::ArraySlice<u64> state, u64 seed) {}
};

template <int N, int Size, int Order>
void FeatureAssignImpl<N, Size, Order>::assignFeature(util::MutableArraySlice<u32> result, util::ArraySlice<u64> state, u64 seed) {
  FieldAssignImpl<Order, Size, N>::assignFeatureStep(result, state, seed);
  FeatureAssignImpl<N - 1, Size, Order>::assignFeature(result, state, seed);
}

template <int N, int Size, int Order>
struct FeatureAssigner {
    static JPP_ALWAYS_INLINE void assignFeature(util::MutableArraySlice<u32> result, util::ArraySlice<u64> state, u64 seed) {
      FeatureAssignImpl<N, Size, Order>::assignFeature(result, state, seed);
    }
};

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

    void __attribute__ ((noinline)) computeFeature(util::MutableArraySlice<u32> result, i32 item, u64 seed) {
      util::ConstSliceable<u64> stateSlice{state, numItems, numFeatures};
      FeatureAssigner<30, 30, 3>::assignFeature(result, stateSlice.row(item), seed);
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
      int  i = 0;
      for (; i < idxes.size(); i += Step) {
        r1 += weights[idxes[i + 0] & mask];
        r2 += weights[idxes[i + 1] & mask];
      }
      if (i > idxes.size()) {
        i -= 2;
        switch (i & (Step - 1)) {
          case 1:
            r1 += weights[idxes[i + 0] & mask];
          default:
            ;
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
      int  i = 0;
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
          default:
            ;
        }
      }

      return r1 + r2 + r3 + r4;
    }
};

template <int N>
static void __attribute__ ((noinline)) horizontalPerc(util::MutableArraySlice<float> result, util::ConstSliceable<u32> features, u32 max) {
  u32 mask = max - 1;
  for (int i = 0; i < features.numRows(); ++i) {
    result[i] += HorizontalPerceptron<N>::compute(features.row(i), mask);
  }
}

struct OutputData {
    std::vector<u32> outputFeatures;
    std::vector<float> outputScores;
    util::Sliceable<u32> features{&outputFeatures, numNgrams, numItems};

    OutputData(): outputFeatures(numItems * numNgrams), outputScores(numItems) {}

    void computeFeatures(u64 seed) {
      for (int i = 0; i < features.numRows(); ++i) {
        inputs.computeFeature(features.row(i), i, seed);
      }
    }
};


BENCHMARK("hor-1-4m", [](context* ctx) {
    OutputData out;
    for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
      out.computeFeatures(iter);
      horizontalPerc<1>(&out.outputScores, out.features, 4 * 1024 * 1024);
    }
});

BENCHMARK("hor-2-4m", [](context* ctx) {
    OutputData out;
    for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
      out.computeFeatures(iter);
      horizontalPerc<2>(&out.outputScores, out.features, 4 * 1024 * 1024);
    }
});

BENCHMARK("hor-4-4m", [](context* ctx) {
    OutputData out;
    for (u64 iter = 0; iter < ctx->num_iterations(); ++iter) {
      out.computeFeatures(iter);
      horizontalPerc<4>(&out.outputScores, out.features, 4 * 1024 * 1024);
    }
});