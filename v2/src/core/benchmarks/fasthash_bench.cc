//
// Created by Arseny Tolmachev on 2017/10/14.
//

#define BENCHPRESS_CONFIG_MAIN

#include <xmmintrin.h>
#include <random>
#include <util/array_slice.h>
#include <util/sliceable_array.h>
#include "benchpress/benchpress.hpp"
#include "util/seahash.h"
#include "util/seahash_simd.h"

using context = benchpress::context;
using namespace jumanpp;

volatile u32 numItems = 50;
volatile u32 numNgrams = 32;
volatile u32 maxFeature = (1 << 30);

using hb = jumanpp::util::hashing::SeaHashLite;
using h1 = jumanpp::util::hashing::FastHash1;


const u32 offsets[] = {
    7, 12, 31, 0,
    11, 0, 5, 6,
    21, 19, 16, 5,
    8, 13, 4, 1,
    11, 21, 15, 17,
    8, 4, 11, 15,
    19, 3, 6, 2,
    31, 11, 15, 6,
};

static_assert(sizeof(offsets)/sizeof(u32) == 32, "numOffsets");

JPP_ALWAYS_INLINE void hashSeahashLoop(util::ArraySlice<u64> state, util::ArraySlice<u64> data, util::MutableArraySlice<u32> result, u32 mask) {
  for (int i = 0; i < state.size(); ++i) {
    result.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask;
  }
}

JPP_ALWAYS_INLINE void hashLoop(util::ArraySlice<u64> state, util::ArraySlice<u64> data, util::MutableArraySlice<u32> result, u32 mask) {
  for (int i = 0; i < state.size(); ++i) {
    result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask;
  }
}

JPP_ALWAYS_INLINE void hashUnrolled(util::ArraySlice<u64> state, util::ArraySlice<u64> data, util::MutableArraySlice<u32> result, u32 mask) {
  u32 i = 0;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;

  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;

  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;

  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
}

JPP_ALWAYS_INLINE void hashSeahashUnrolled(util::ArraySlice<u64> state, util::ArraySlice<u64> data, util::MutableArraySlice<u32> finish, u32 mask) {
  u32 i = 0;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;

  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;

  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;

  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
  finish.at(i) = static_cast<u32>(hb{state[i]}.mix(data[offsets[i]]).finish()) & mask; ++i;
}

#ifdef JPP_SSE_IMPL

using h2 = jumanpp::util::hashing::FastHash2;

JPP_ALWAYS_INLINE void hashSseUnrolled(util::ArraySlice<u64> state, util::ArraySlice<u64> data, util::MutableArraySlice<u32> result, u32 mask) {
  u32 i = 0;
  {
    auto r = i;
    auto s1 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    auto s2 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    s1.jointMaskStore(s2, mask, result.data() + r);
  }
  {
    auto r = i;
    auto s1 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    auto s2 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    s1.jointMaskStore(s2, mask, result.data() + r);
  }

  {
    auto r = i;
    auto s1 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    auto s2 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    s1.jointMaskStore(s2, mask, result.data() + r);
  }
  {
    auto r = i;
    auto s1 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    auto s2 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    s1.jointMaskStore(s2, mask, result.data() + r);
  }

  {
    auto r = i;
    auto s1 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    auto s2 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    s1.jointMaskStore(s2, mask, result.data() + r);
  }
  {
    auto r = i;
    auto s1 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    auto s2 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    s1.jointMaskStore(s2, mask, result.data() + r);
  }

  {
    auto r = i;
    auto s1 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    auto s2 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    s1.jointMaskStore(s2, mask, result.data() + r);
  }
  {
    auto r = i;
    auto s1 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    auto s2 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    s1.jointMaskStore(s2, mask, result.data() + r);
  }
}

JPP_ALWAYS_INLINE void hashSseInterleaved(util::ArraySlice<u64> state, util::ArraySlice<u64> data, util::MutableArraySlice<u32> result, u32 mask) {

  u32 i = 0;

  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;

  {
    auto r = i;
    auto s1 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    auto s2 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    s1.jointMaskStore(s2, mask, result.data() + r);
  }

  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;

  {
    auto r = i;
    auto s1 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    auto s2 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    s1.jointMaskStore(s2, mask, result.data() + r);
  }

  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;

  {
    auto r = i;
    auto s1 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    auto s2 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    s1.jointMaskStore(s2, mask, result.data() + r);
  }

  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;

  {
    auto r = i;
    auto s1 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    auto s2 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    s1.jointMaskStore(s2, mask, result.data() + r);
  }

  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;

  {
    auto r = i;
    auto s1 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    auto s2 = h2{state.data() + i}.mixGather(data.data() + i, offsets + i);
    i += 2;
    s1.jointMaskStore(s2, mask, result.data() + r);
  }

  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
}

#endif //SSE

#ifdef JPP_AVX2

using h4 = jumanpp::util::hashing::FastHash4;

JPP_ALWAYS_INLINE void hashAvxUnrolled(util::ArraySlice<u64> state, util::ArraySlice<u64> data, util::MutableArraySlice<u32> result, u32 mask) {
  u32 i = 0;
  {
    auto r = i;
    auto s1 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    auto s2 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    s1.merge(s2).store(result.data() + r);
  }
  {
    auto r = i;
    auto s1 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    auto s2 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    s1.merge(s2).store(result.data() + r);
  }
  {
    auto r = i;
    auto s1 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    auto s2 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    s1.merge(s2).store(result.data() + r);
  }
  {
    auto r = i;
    auto s1 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    auto s2 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    s1.merge(s2).store(result.data() + r);
  }
}

JPP_ALWAYS_INLINE void hashAvxUnrolledInterleave1(util::ArraySlice<u64> state, util::ArraySlice<u64> data, util::MutableArraySlice<u32> result, u32 mask) {
  u32 i = 0;

  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;

  {
    auto r = i;
    auto s1 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    auto s2 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    s1.merge(s2).store(result.data() + r);
  }

  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;

  {
    auto r = i;
    auto s1 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    auto s2 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    s1.merge(s2).store(result.data() + r);
  }

  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;

  {
    auto r = i;
    auto s1 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    auto s2 = h4{state.data() + i}.mixGatherNaive(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    s1.merge(s2).store(result.data() + r);
  }

  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
  result.at(i) = static_cast<u32>(h1{state.data() + i}.mix(data.data() + offsets[i]).result()) & mask; ++i;
}

JPP_ALWAYS_INLINE void hashAvxUnrolledIgather(util::ArraySlice<u64> state, util::ArraySlice<u64> data, util::MutableArraySlice<u32> result, u32 mask) {
  u32 i = 0;
  {
    auto r = i;
    auto s1 = h4{state.data() + i}.mixGatherInstr(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    auto s2 = h4{state.data() + i}.mixGatherInstr(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    s1.merge(s2).store(result.data() + r);
  }
  {
    auto r = i;
    auto s1 = h4{state.data() + i}.mixGatherInstr(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    auto s2 = h4{state.data() + i}.mixGatherInstr(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    s1.merge(s2).store(result.data() + r);
  }
  {
    auto r = i;
    auto s1 = h4{state.data() + i}.mixGatherInstr(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    auto s2 = h4{state.data() + i}.mixGatherInstr(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    s1.merge(s2).store(result.data() + r);
  }
  {
    auto r = i;
    auto s1 = h4{state.data() + i}.mixGatherInstr(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    auto s2 = h4{state.data() + i}.mixGatherInstr(data.data() + i, offsets + i).maskCompact2(mask);
    i += 4;
    s1.merge(s2).store(result.data() + r);
  }
}

#endif

struct InputData {
  std::vector<u64> state;
  std::vector<u64> data;
  std::vector<u32> features;
  util::Sliceable<u32> featuresSlice{&features, numNgrams, numItems};

  InputData(): features(numItems * numNgrams, 0xdedbeef0u) {
    std::minstd_rand rng{1};

    std::uniform_int_distribution<u64> states{};
    u32 totalStates = numItems * numNgrams;
    state.reserve(totalStates);
    for (int i = 0; i < totalStates; ++i) {
      state.push_back(states(rng));
      data.push_back(states(rng));
    }
  }

} inputs;

template <typename Fn>
void doTest(util::Sliceable<u32> result, u32 mask, Fn f) {
  auto& state = inputs.state;
  auto& data = inputs.data;
  u32 nitems = numItems;
  util::ConstSliceable<u64> stateSlice{state, numNgrams, numItems};
  util::ConstSliceable<u64> dataSlice{data, numNgrams, numItems};
  for (int i = 0; i < nitems; ++i) {
    f(stateSlice.row(i), dataSlice.row(i), result.row(i), mask);
  }
}

__attribute__((noinline)) void test_naiveSeahashLoop(util::Sliceable<u32> result, u32 mask) {
  doTest(result, mask, hashSeahashLoop);
}

BENCHMARK("sealite-loop", [](context* ctx) {
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    test_naiveSeahashLoop(inputs.featuresSlice, maxFeature - 1u);
  }
});

__attribute__((noinline)) void test_unrolledSeahashLoop(util::Sliceable<u32> result, u32 mask) {
  doTest(result, mask, hashSeahashUnrolled);
}

BENCHMARK("sealite-loop", [](context* ctx) {
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    test_unrolledSeahashLoop(inputs.featuresSlice, maxFeature - 1u);
  }
});

__attribute__((noinline)) void test_naiveLoop(util::Sliceable<u32> result, u32 mask) {
  doTest(result, mask, hashLoop);
}

BENCHMARK("naive-loop", [](context* ctx) {
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    test_naiveLoop(inputs.featuresSlice, maxFeature - 1u);
  }
});

__attribute__((noinline)) void test_naiveUnrolled(util::Sliceable<u32> result, u32 mask) {
  doTest(result, mask, hashUnrolled);
}

BENCHMARK("naive-unrolled", [](context* ctx) {
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    test_naiveUnrolled(inputs.featuresSlice, maxFeature - 1u);
  }
});

#ifdef JPP_SSE_IMPL

__attribute__((noinline)) void test_sseUnrolled(util::Sliceable<u32> result, u32 mask) {
  doTest(result, mask, hashSseUnrolled);
}

BENCHMARK("sse-unrolled", [](context* ctx) {
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    test_sseUnrolled(inputs.featuresSlice, maxFeature - 1u);
  }
});

__attribute__((noinline)) void test_sseInterleaved(util::Sliceable<u32> result, u32 mask) {
  doTest(result, mask, hashSseInterleaved);
}

BENCHMARK("sse-interleaved", [](context* ctx) {
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    test_sseInterleaved(inputs.featuresSlice, maxFeature - 1u);
  }
});

#endif

#ifdef JPP_AVX2

__attribute__((noinline)) void test_avxUnrolled(util::Sliceable<u32> result, u32 mask) {
  doTest(result, mask, hashAvxUnrolled);
}

BENCHMARK("avx-unrolled-naive", [](context* ctx) {
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    test_avxUnrolled(inputs.featuresSlice, maxFeature - 1u);
  }
});

__attribute__((noinline)) void test_avxUnrolledInterleave(util::Sliceable<u32> result, u32 mask) {
  doTest(result, mask, hashAvxUnrolledInterleave1);
}

BENCHMARK("avx-unrolled-interleave", [](context* ctx) {
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    test_avxUnrolledInterleave(inputs.featuresSlice, maxFeature - 1u);
  }
});

__attribute__((noinline)) void test_avxUnrolledIGather(util::Sliceable<u32> result, u32 mask) {
  doTest(result, mask, hashAvxUnrolledIgather);
}

BENCHMARK("avx-unrolled-instr", [](context* ctx) {
  for (size_t i = 0; i < ctx->num_iterations(); ++i) {
    test_avxUnrolledIGather(inputs.featuresSlice, maxFeature - 1u);
  }
});

#endif // JPP_AVX2