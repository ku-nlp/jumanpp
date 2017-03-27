//
// Created by Arseny Tolmachev on 2017/03/05.
//

#ifndef JUMANPP_SPEC_SER_H
#define JUMANPP_SPEC_SER_H

#include "spec_types.h"
#include "util/serialization.h"

namespace jumanpp {
namespace chars {
SERIALIZE_ENUM_CLASS(CharacterClass, i32);
}

namespace core {
namespace spec {

SERIALIZE_ENUM_CLASS(ColumnType, int);

template <typename Arch>
void Serialize(Arch &a, FieldDescriptor &fd) {
  a &fd.index;
  a &fd.position;
  a &fd.name;
  a &fd.isTrieKey;
  a &fd.columnType;
  a &fd.emptyString;
  a &fd.stringStorage;
  a &fd.intStorage;
}

template <typename Arch>
void Serialize(Arch &a, DictionarySpec &spec) {
  a &spec.columns;
  a &spec.indexColumn;
  a &spec.numIntStorage;
  a &spec.numStringStorage;
}

SERIALIZE_ENUM_CLASS(UnkMakerType, int);

SERIALIZE_ENUM_CLASS(FieldExpressionKind, int);

template <typename Arch>
void Serialize(Arch &a, FieldExpression &o) {
  a &o.fieldIndex;
  a &o.kind;
  a &o.intConstant;
  a &o.stringConstant;
}

SERIALIZE_ENUM_CLASS(UnkFeatureType, int);

template <typename Arch>
void Serialize(Arch &a, UnkMakerFeature &o) {
  a &o.type;
  a &o.reference;
}

template <typename Arch>
void Serialize(Arch &a, UnkMaker &o) {
  a &o.index;
  a &o.name;
  a &o.type;
  a &o.patternRow;
  a &o.charClass;
  a &o.features;
  a &o.outputExpressions;
}

SERIALIZE_ENUM_CLASS(PrimitiveFeatureKind, int);

template <typename Arch>
void Serialize(Arch &a, PrimitiveFeatureDescriptor &o) {
  a &o.index;
  a &o.name;
  a &o.kind;
  a &o.references;
  a &o.matchData;
}

template <typename Arch>
void Serialize(Arch &a, MatchReference &o) {
  a &o.featureIdx;
  a &o.dicFieldIdx;
}

template <typename Arch>
void Serialize(Arch &a, ComputationFeatureDescriptor &o) {
  a &o.index;
  a &o.name;
  a &o.matchReference;
  a &o.matchData;
  a &o.trueBranch;
  a &o.falseBranch;
}

template <typename Arch>
void Serialize(Arch &a, PatternFeatureDescriptor &o) {
  a &o.index;
  a &o.references;
}

template <typename Arch>
void Serialize(Arch &a, FinalFeatureDescriptor &o) {
  a &o.index;
  a &o.references;
}

template <typename Arch>
void Serialize(Arch &a, FeaturesSpec &o) {
  a &o.primitive;
  a &o.computation;
  a &o.pattern;
  a &o.final;
  a &o.totalPrimitives;
}

template <typename Arch>
void Serialize(Arch &a, TrainingField &o) {
  a &o.index;
  a &o.weight;
}

template <typename Arch>
void Serialize(Arch &a, TrainingSpec &o) {
  a &o.fields;
  a &o.surfaceIdx;
}

template <typename Arch>
void Serialize(Arch &a, AnalysisSpec &spec) {
  a &spec.dictionary;
  a &spec.features;
  a &spec.unkCreators;
  a &spec.training;
}

}  // spec
}  // core
}  // jumanpp

#endif  // JUMANPP_SPEC_SER_H
