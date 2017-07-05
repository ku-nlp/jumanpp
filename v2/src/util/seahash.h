//
// Created by Arseny Tolmachev on 2017/05/29.
//

#ifndef JUMANPP_SEAHASH_H
#define JUMANPP_SEAHASH_H

#include "common.hpp"
#include "types.hpp"

namespace jumanpp {
namespace util {
namespace hashing {

static constexpr u64 SeaHashSeed0 = 0x16f11fe89b0d677cULL;
static constexpr u64 SeaHashSeed1 = 0xb480a793d8e6c86cULL;

// See http://ticki.github.io/blog/seahash-explained/ for explanation.
// This implementation uses only two u64 for states instead of four.

struct SeaHashState {
  u64 s0 = SeaHashSeed0;
  u64 s1 = SeaHashSeed1;

  static SeaHashState withSeed(u64 seed) {
    return SeaHashState{SeaHashSeed0 ^ diffuse(seed),
                        SeaHashSeed1 ^ diffuse(seed)};
  }

  JPP_ALWAYS_INLINE inline static u64 diffuse(u64 v) {
    v *= 0x6eed0e9da4d94a4fULL;
    auto a = v >> 32;
    auto b = static_cast<unsigned char>(v >> 60);
    v ^= a >> b;
    v *= 0x6eed0e9da4d94a4fULL;
    return v;
  }

  JPP_ALWAYS_INLINE inline SeaHashState mix(u64 v1, u64 v2) const noexcept {
    return SeaHashState{diffuse(s0 ^ v1), diffuse(s1 ^ v2)};
  }

  JPP_ALWAYS_INLINE inline SeaHashState mix(u64 v0) {
    return SeaHashState{s1, diffuse(v0 ^ s0)};
  }

  JPP_ALWAYS_INLINE inline u64 finish() const noexcept {
    return diffuse(s0 ^ s1);
  }
};

namespace detail {
JPP_ALWAYS_INLINE inline SeaHashState seaHashSeqImpl(SeaHashState h) {
  return h;
}

JPP_ALWAYS_INLINE inline SeaHashState seaHashSeqImpl(SeaHashState h, u64 one) {
  return h.mix(one);
}

template <typename... Args>
JPP_ALWAYS_INLINE inline SeaHashState seaHashSeqImpl(SeaHashState h, u64 one,
                                                     u64 two, Args... args) {
  return seaHashSeqImpl(h.mix(one, two), args...);
}

template <>
JPP_ALWAYS_INLINE inline SeaHashState seaHashSeqImpl(SeaHashState h, u64 one,
                                                     u64 two) {
  return h.mix(one, two);
}
}  // namespace detail

/**
 * Hash sequence with compile-time passed parameters.
 * This version pushes tuple size as a first argument.
 *
 * Implementation uses C++11 vararg templates.
 * @param seed seed value for hash calcuation
 * @param args values for hash calculation
 * @return hash value
 */
template <typename... Args>
JPP_ALWAYS_INLINE inline u64 seaHashSeq(Args... args) {
  return detail::seaHashSeqImpl(SeaHashState{}, sizeof...(args),
                                static_cast<u64>(args)...)
      .finish();
}

// This version pushes tuple size as a last argument.
template <typename... Args>
JPP_ALWAYS_INLINE inline u64 seaHashSeq2(Args... args) {
  return detail::seaHashSeqImpl(SeaHashState{}, static_cast<u64>(args)...,
                                sizeof...(args))
      .finish();
}

template <typename Seq, typename Idx>
inline u64 seaHashIndexedSeq(u64 seed, const Seq& seq, const Idx& idx) {
  auto numIndices = idx.size();
  SeaHashState state{numIndices};
  state = state.mix(seed);
  auto steps2 = numIndices / 2;
  for (int i = 0; i < steps2; ++i) {
    state = state.mix(seq[idx[i]], seq[idx[i + 1]]);
  }
  if ((numIndices & 0x1) != 0) {
    state = state.mix(seq[idx[numIndices - 1]]);
  }
  return state.finish();
}

}  // namespace hashing
}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_SEAHASH_H
