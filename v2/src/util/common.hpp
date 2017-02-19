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
#define JPP_DCHECK_EQ(a, b) (assert(((a) == (b)) && (#a "was not ==" #b)))
#else
#define JPP_DCHECK_EQ(a, b) ()
#endif

#endif  // JUMANPP_COMMON_HPP
