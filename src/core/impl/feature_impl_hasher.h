//
// Created by eienn on 2021/07/09.
//

#ifndef JUMANPP_FEATURE_IMPL_HASHER_H
#define JUMANPP_FEATURE_IMPL_HASHER_H

#include "util/fast_hash.h"
#include "util/fast_hash_rot.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

// This is a typedef which specifies a hasher implementation
// to use in Juman++
//using Hasher = util::hashing::FastHash1;
using Hasher = util::hashing::FastHashRot;

}
}
}
}

#endif  // JUMANPP_FEATURE_IMPL_HASHER_H
