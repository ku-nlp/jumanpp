//
// Created by Arseny Tolmachev on 2017/03/05.
//

#ifndef JUMANPP_RUNTIME_SER_H
#define JUMANPP_RUNTIME_SER_H

#include "core/runtime_info.h"
#include "core/spec/spec_ser.h"
#include "util/serialization.h"

namespace jumanpp {
namespace core {

namespace analysis {

template <typename Arch>
void Serialize(Arch& a, UnkMakerInfo& o) {
  a& o.index;
  a& o.name;
  a& o.type;
  a& o.patternPtr;
  a& o.charClass;
  a& o.features;
  a& o.output;
}

template <typename Arch>
void Serialize(Arch& a, UnkMakersInfo& o) {
  a& o.numPlaceholders;
  a& o.makers;
}
}  // namespace analysis

namespace features {

template <typename Arch>
void Serialize(Arch& a, PrimitiveFeature& o) {
  a& o.name;
  a& o.index;
  a& o.kind;
  a& o.references;
  a& o.matchData;
}

template <typename Arch>
void Serialize(Arch& a, ComputeFeature& o) {
  a& o.name;
  a& o.index;
  a& o.references;
  a& o.matchData;
  a& o.trueBranch;
  a& o.falseBranch;
}

template <typename Arch>
void Serialize(Arch& a, PatternFeature& o) {
  a& o.index;
  a& o.arguments;
}

template <typename Arch>
void Serialize(Arch& a, NgramFeature& o) {
  a& o.index;
  a& o.arguments;
}

template <typename Arch>
void Serialize(Arch& a, FeatureRuntimeInfo& o) {
  a& o.primitive;
  a& o.compute;
  a& o.patterns;
  a& o.ngrams;
  a& o.placeholderMapping;
}

}  // namespace features

template <typename Arch>
void Serialize(Arch& a, RuntimeInfo& o) {
  a& o.features;
  a& o.unkMakers;
}

}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_RUNTIME_SER_H
