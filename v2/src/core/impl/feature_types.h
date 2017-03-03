//
// Created by Arseny Tolmachev on 2017/02/27.
//

#ifndef JUMANPP_FEATURE_TYPES_H
#define JUMANPP_FEATURE_TYPES_H

#include "core/spec/spec_types.h"
#include "util/array_slice.h"
#include "util/status.hpp"
#include "util/string_piece.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace features {

using spec::PrimitiveFeatureKind;

struct PrimitiveFeature {
  StringPiece name;
  i32 index = -1;
  PrimitiveFeatureKind kind;
  util::ArraySlice<i32> references;
  util::ArraySlice<i32> matchData;
};

struct ComputeFeature {
  StringPiece name;
  i32 index = -1;
  util::ArraySlice<spec::MatchReference> references;
  util::ArraySlice<i32> matchData;
  util::ArraySlice<i32> trueBranch;
  util::ArraySlice<i32> falseBranch;
};

struct PatternFeature {
  StringPiece name;
  i32 index;
  util::ArraySlice<i32> arguments;
};

}  // features
}  // core
}  // jumanpp

#endif  // JUMANPP_FEATURE_TYPES_H
