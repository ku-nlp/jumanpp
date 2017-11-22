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
  std::vector<i32> references;
  std::vector<i32> matchData;
};

struct ComputeFeature {
  StringPiece name;
  i32 index = -1;
  std::vector<i32> references;
  std::vector<i32> matchData;
  std::vector<i32> trueBranch;
  std::vector<i32> falseBranch;
};

struct PatternFeature {
  i32 index;
  std::vector<i32> arguments;
};

struct NgramFeature {
  i32 index;
  std::vector<i32> arguments;
};

struct FeatureRuntimeInfo {
  std::vector<features::PrimitiveFeature> primitive;
  std::vector<features::ComputeFeature> compute;
  std::vector<features::PatternFeature> patterns;
  std::vector<features::NgramFeature> ngrams;
  std::vector<i32> placeholderMapping;
};

}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_TYPES_H
