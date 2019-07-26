//
// Created by Arseny Tolmachev on 2019-07-24.
//

#ifndef JUMANPP_SPEC_HASHING_H
#define JUMANPP_SPEC_HASHING_H

#include "util/fast_hash.h"
#include "util/murmur_hash.h"
#include "util/string_piece.h"

namespace jumanpp {
namespace core {
namespace spec {

struct AnalysisSpec;

u64 hashSpec(const AnalysisSpec& spec);

}  // namespace spec
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SPEC_HASHING_H
