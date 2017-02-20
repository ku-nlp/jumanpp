//
// Created by Arseny Tolmachev on 2017/02/17.
//

#ifndef JUMANPP_COMMON_HPP
#define JUMANPP_COMMON_HPP

#include <cassert>

#define JPP_ALWAYS_INLINE __attribute__((always_inline))

#define JPP_LIKELY(x) __builtin_expect((x), 1)
#define JPP_UNLIKELY(x) __builtin_expect((x), 0)

#ifndef NDEBUG
#define JPP_DCHECK(x) (assert(x))
#else
#define JPP_DCHECK(x)
#endif

#define JPP_DCHECK_EQ(a, b) JPP_DCHECK((a) == (b))
#define JPP_DCHECK_NE(a, b) JPP_DCHECK((a) != (b))
#define JPP_DCHECK_GE(a, b) JPP_DCHECK((a) >= (b))
#define JPP_DCHECK_GT(a, b) JPP_DCHECK((a) > (b))
#define JPP_DCHECK_LE(a, b) JPP_DCHECK((a) <= (b))
#define JPP_DCHECK_LT(a, b) JPP_DCHECK((a) < (b))
#define JPP_DCHECK_NOT(x) JPP_DCHECK(!(x))

namespace jumanpp {
namespace util {

// Prefetching support
//
// Defined behavior on some of the uarchs:
// PREFETCH_HINT_T0:
//   prefetch to all levels of the hierarchy (except on p4: prefetch to L2)
// PREFETCH_HINT_NTA:
//   p4: fetch to L2, but limit to 1 way (out of the 8 ways)
//   core: skip L2, go directly to L1
//   k8 rev E and later: skip L2, can go to either of the 2-ways in L1
// Imported from TensorFlow.
enum PrefetchHint {
  PREFETCH_HINT_T0 = 3,  // More temporal locality
  PREFETCH_HINT_T1 = 2,
  PREFETCH_HINT_T2 = 1,  // Less temporal locality
  PREFETCH_HINT_NTA = 0  // No temporal locality
};
template <PrefetchHint hint>
JPP_ALWAYS_INLINE void prefetch(const void* x);

// ---------------------------------------------------------------------------
// Inline implementation
// ---------------------------------------------------------------------------
template <PrefetchHint hint>
JPP_ALWAYS_INLINE inline void prefetch(const void* x) {
#if defined(__llvm__) || defined(COMPILER_GCC)
  __builtin_prefetch(x, 0, hint);
#else
// You get no effect.  Feel free to add more sections above.
#endif
}

namespace port {
constexpr bool kLittleEndian = false;
}

}  // util
}  // jumanpp

#endif  // JUMANPP_COMMON_HPP
