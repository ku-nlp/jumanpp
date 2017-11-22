//
// Created by Arseny Tolmachev on 2017/11/15.
//

#define BENCHPRESS_CONFIG_MAIN

#include <core/analysis/perceptron.h>
#include <benchpress/benchpress.hpp>
#include <random>
#include "util/common.hpp"
#include "util/fast_hash.h"
#include "util/sliceable_array.h"
#include "util/types.hpp"

using namespace jumanpp;

volatile u32 numNgrams = 32;
volatile u32 numItems = 50;
volatile u32 numStates = 500;

JPP_ALWAYS_INLINE u32 doMix(u64 state, u64 val, u32 mask) {
  return util::hashing::FastHash1{state}.mix(val).masked(mask);
}

JPP_NO_INLINE void unrollFull4NoPrefetch(
    util::Sliceable<u64> state, util::Sliceable<u64> data,
    util::ArraySlice<float> weights, util::MutableArraySlice<float> result) {
  float f1 = 0, f2 = 0, f3 = 0, f4 = 0;
  u32 mask = static_cast<u32>(weights.size() - 1);
  for (int row = 0; row < data.numRows(); ++row) {
    auto sr = state.row(row);
    auto dr = data.row(row);
    u32 id = 0;

    //clang-format off
    f1 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f2 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f3 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f4 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;

    f1 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f2 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f3 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f4 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;

    f1 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f2 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f3 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f4 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;

    f1 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f2 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f3 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f4 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;

    f1 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f2 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f3 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f4 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;

    f1 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f2 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f3 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f4 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;

    f1 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f2 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f3 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f4 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;

    f1 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f2 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f3 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;
    f4 += weights.at(doMix(sr.at(id), dr.at(id), mask));
    ++id;

    result.at(row) = f1 + f2 + f3 + f4;
    f1 = f2 = f3 = f4 = 0;
    //clang-format on
  }
}

JPP_NO_INLINE void unrollFull4Prefetch(util::Sliceable<u64> state,
                                       util::Sliceable<u64> data,
                                       util::ArraySlice<float> weights,
                                       util::MutableArraySlice<float> result,
                                       util::MutableArraySlice<u32> buf1,
                                       util::MutableArraySlice<u32> buf2) {
  float f1 = 0, f2 = 0, f3 = 0, f4 = 0;
  u32 mask = static_cast<u32>(weights.size() - 1);
  for (int row = 0; row < data.numRows(); ++row) {
    auto sr = state.row(row);
    auto dr = data.row(row);
    u32 id = 0;
    buf1.swap(buf2);

    //clang-format off
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f1 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f2 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f3 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f4 += weights.at(buf2.at(id));
      ++id;
    }

    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f1 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f2 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f3 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f4 += weights.at(buf2.at(id));
      ++id;
    }

    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f1 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f2 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f3 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f4 += weights.at(buf2.at(id));
      ++id;
    }

    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f1 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f2 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f3 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f4 += weights.at(buf2.at(id));
      ++id;
    }

    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f1 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f2 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f3 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f4 += weights.at(buf2.at(id));
      ++id;
    }

    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f1 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f2 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f3 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f4 += weights.at(buf2.at(id));
      ++id;
    }

    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f1 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f2 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f3 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f4 += weights.at(buf2.at(id));
      ++id;
    }

    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f1 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f2 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f3 += weights.at(buf2.at(id));
      ++id;
    }
    {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      buf1.at(id) = v;
      f4 += weights.at(buf2.at(id));
      ++id;
    }

    if (JPP_LIKELY(row > 0)) {
      result.at(row - 1) = f1 + f2 + f3 + f4;
    }
    f1 = f2 = f3 = f4 = 0;
    //clang-format on
  }
  result.at(data.numRows() - 1) =
      core::analysis::impl::computeUnrolled4RawPerceptron(weights, buf1);
}

JPP_NO_INLINE void noUnrollNoPrefetch(util::Sliceable<u64> state,
                                      util::Sliceable<u64> data,
                                      util::ArraySlice<float> weights,
                                      util::MutableArraySlice<float> result) {
  u32 mask = static_cast<u32>(weights.size() - 1);
  for (int row = 0; row < data.numRows(); ++row) {
    auto sr = state.row(row);
    auto dr = data.row(row);
    float f = 0;
    for (u32 id = 0; id < 32; ++id) {
      f += weights.at(doMix(sr.at(id), dr.at(id), mask));
    }
    result.at(row) = f;
  }
}

JPP_NO_INLINE void noUnrollPrefetch(util::Sliceable<u64> state,
                                    util::Sliceable<u64> data,
                                    util::ArraySlice<float> weights,
                                    util::MutableArraySlice<float> result,
                                    util::MutableArraySlice<u32> buf1,
                                    util::MutableArraySlice<u32> buf2) {
  u32 mask = static_cast<u32>(weights.size() - 1);
  for (int row = 0; row < data.numRows(); ++row) {
    auto sr = state.row(row);
    auto dr = data.row(row);
    buf1.swap(buf2);
    float f = 0;
    for (u32 id = 0; id < 32; ++id) {
      auto v = doMix(sr.at(id), dr.at(id), mask);
      buf1.at(id) = v;
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      f += weights.at(buf2.at(id));
    }
    if (JPP_LIKELY(row > 0)) {
      result.at(row - 1) = f;
    }
  }
  result.at(data.numRows() - 1) =
      core::analysis::impl::computeUnrolled4RawPerceptron(weights, buf1);
}

JPP_NO_INLINE void partUnroll2NoPrefetch(
    util::Sliceable<u64> state, util::Sliceable<u64> data,
    util::ArraySlice<float> weights, util::MutableArraySlice<float> result) {
  u32 mask = static_cast<u32>(weights.size() - 1);
  for (int row = 0; row < data.numRows(); ++row) {
    auto sr = state.row(row);
    auto dr = data.row(row);
    float f1 = 0;
    float f2 = 0;
    for (u32 id0 = 0; id0 < 32; id0 += 2) {
      auto id = id0;
      auto v = doMix(sr.at(id), dr.at(id), mask);
      f1 += weights.at(v);
      ++id;
      auto v2 = doMix(sr.at(id), dr.at(id), mask);
      f2 += weights.at(v2);
    }
    result.at(row) = f1 + f2;
  }
}

JPP_NO_INLINE void partUnroll2Prefetch(util::Sliceable<u64> state,
                                       util::Sliceable<u64> data,
                                       util::ArraySlice<float> weights,
                                       util::MutableArraySlice<float> result,
                                       util::MutableArraySlice<u32> buf1,
                                       util::MutableArraySlice<u32> buf2) {
  u32 mask = static_cast<u32>(weights.size() - 1);
  for (int row = 0; row < data.numRows(); ++row) {
    auto sr = state.row(row);
    auto dr = data.row(row);
    buf1.swap(buf2);
    float f1 = 0;
    float f2 = 0;
    for (u32 id0 = 0; id0 < 32; id0 += 2) {
      auto id = id0;
      auto v = doMix(sr.at(id), dr.at(id), mask);
      buf1.at(id) = v;
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v));
      f1 += weights.at(buf2.at(id));
      ++id;
      auto v2 = doMix(sr.at(id), dr.at(id), mask);
      buf1.at(id) = v2;
      util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(&weights.at(v2));
      f2 += weights.at(buf2.at(id));
    }
    if (JPP_LIKELY(row > 0)) {
      result.at(row - 1) = f1 + f2;
    }
  }
  result.at(data.numRows() - 1) =
      core::analysis::impl::computeUnrolled4RawPerceptron(weights, buf1);
}

struct InputData {
  std::vector<u64> state;
  std::vector<u64> data;
  std::vector<u32> buffer1;
  std::vector<u32> buffer2;
  std::vector<float> weights;
  std::vector<float> result;

  JPP_NO_INLINE InputData() {
    std::minstd_rand rng{1};

    std::uniform_int_distribution<u64> states{};
    u32 totalStates = numItems * numNgrams * numStates;
    state.reserve(totalStates);
    for (int i = 0; i < totalStates; ++i) {
      state.push_back(states(rng));
    }
    for (int i = 0; i < numNgrams * numItems; ++i) {
      data.push_back(states(rng));
    }
    weights.reserve(4 * 1024 * 1024);
    std::uniform_real_distribution<float> fdist;
    for (int i = 0; i < 4 * 1024 * 1024; ++i) {
      weights.push_back(fdist(rng));
    }
    result.resize(numItems);
    buffer1.resize(numNgrams, 0);
    buffer2.resize(numNgrams, 0);
  }

  util::Sliceable<u64> states(u32 num) {
    auto stateSize = numNgrams * numItems;
    util::MutableArraySlice<u64> stateData{&state, num * stateSize, stateSize};
    return util::Sliceable<u64>{stateData, numNgrams, numItems};
  }

  util::Sliceable<u64> datas() {
    auto stateSize = numNgrams * numItems;
    util::MutableArraySlice<u64> stateData{&data, 0, stateSize};
    return util::Sliceable<u64>{stateData, numNgrams, numItems};
  }

} inputs;

BENCHMARK("unroll-4-no-prefetch", [](benchpress::context* ctx) {
  InputData data{};
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    for (int nstate = 0; nstate < numStates; ++nstate) {
      unrollFull4NoPrefetch(inputs.states(nstate), inputs.datas(),
                            inputs.weights, &inputs.result);
    }
  }
});

BENCHMARK("no-unroll-no-prefetch", [](benchpress::context* ctx) {
  InputData data{};
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    for (int nstate = 0; nstate < numStates; ++nstate) {
      noUnrollNoPrefetch(inputs.states(nstate), inputs.datas(), inputs.weights,
                         &inputs.result);
    }
  }
});

BENCHMARK("punroll2-no-prefetch", [](benchpress::context* ctx) {
  InputData data{};
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    for (int nstate = 0; nstate < numStates; ++nstate) {
      partUnroll2NoPrefetch(inputs.states(nstate), inputs.datas(),
                            inputs.weights, &inputs.result);
    }
  }
});

BENCHMARK("full-unroll4-prefetch", [](benchpress::context* ctx) {
  InputData data{};
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    for (int nstate = 0; nstate < numStates; ++nstate) {
      unrollFull4Prefetch(inputs.states(nstate), inputs.datas(), inputs.weights,
                          &inputs.result, &inputs.buffer1, &inputs.buffer2);
    }
  }
});

BENCHMARK("no-unroll-prefetch", [](benchpress::context* ctx) {
  InputData data{};
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    for (int nstate = 0; nstate < numStates; ++nstate) {
      noUnrollPrefetch(inputs.states(nstate), inputs.datas(), inputs.weights,
                       &inputs.result, &inputs.buffer1, &inputs.buffer2);
    }
  }
});

BENCHMARK("punroll2-prefetch", [](benchpress::context* ctx) {
  InputData data{};
  ctx->reset_timer();
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    for (int nstate = 0; nstate < numStates; ++nstate) {
      partUnroll2Prefetch(inputs.states(nstate), inputs.datas(), inputs.weights,
                          &inputs.result, &inputs.buffer1, &inputs.buffer2);
    }
  }
});