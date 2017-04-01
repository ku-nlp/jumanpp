//
// Created by Arseny Tolmachev on 2017/03/03.
//

#ifndef JUMANPP_HASHING_H
#define JUMANPP_HASHING_H

#include "murmur_hash.h"

namespace jumanpp {
namespace util {
namespace hashing {

class Hasher {
  impl::OWORD state_;
  constexpr Hasher() = delete;

 public:
  constexpr Hasher(u64 seed) : state_{seed, seed} {}
  constexpr Hasher(u64 seed1, u64 seed2) : state_{seed1, seed2} {}

  inline Hasher merge(u64 one) const {
    Hasher copy(*this);
    impl::OWORD value{one, 0};
    impl::murmur_oword(value, &copy.state_);
    return copy;
  }

  inline Hasher merge(u64 one, u64 two) const {
    Hasher copy(*this);
    impl::OWORD value{one, two};
    impl::murmur_oword(value, &copy.state_);
    return copy;
  }

  inline u64 result() const {
    u64 r1 = impl::fmix64(state_.first);
    u64 r2 = impl::fmix64(state_.second);
    return r1 + r2;
  }
};

template <typename Seq, typename Idx>
inline u64 hashIndexedSeq(u64 seed, const Seq& seq, const Idx& idx) {
  Hasher h{seed};
  auto numIndices = idx.size();
  auto steps2 = numIndices / 2;
  for (int i = 0; i < steps2; ++i) {
    h = h.merge(seq[idx[i]], seq[idx[i + 1]]);
  }
  if ((numIndices & 0x1) != 0) {
    h = h.merge(seq[idx[numIndices - 1]], numIndices);
  } else {
    h = h.merge(numIndices);
  }
  return h.result();
}

namespace impl {
inline Hasher hashCtSeqImpl(Hasher h) { return h; }

inline Hasher hashCtSeqImpl(Hasher h, u64 one) { return h.merge(one); }

template <typename... Args>
inline Hasher hashCtSeqImpl(Hasher h, u64 one, u64 two, Args... args) {
  return hashCtSeqImpl(h.merge(one, two), args...);
}

}  // namespace impl

/**
 * Hash sequence with compile-time passed parameters.
 *
 * Implementation uses C++11 vararg templates.
 * @param seed seed value for hash calcuation
 * @param args values for hash calculation
 * @return hash value
 */
template <typename... Args>
inline u64 hashCtSeq(u64 seed, Args... args) {
  return impl::hashCtSeqImpl(Hasher{seed}, static_cast<u64>(args)...,
                             sizeof...(args))
      .result();
}

}  // namespace hashing
}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_HASHING_H
