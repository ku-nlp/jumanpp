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

SERIALIZE_ENUM_CLASS(FieldType, int);

template <typename Arch>
void Serialize(Arch &a, FieldDescriptor &fd) {
  a &fd.specIndex;
  a &fd.position;
  a &fd.dicIndex;
  a &fd.name;
  a &fd.isTrieKey;
  a &fd.fieldType;
  a &fd.emptyString;
  a &fd.listSeparator;
  a &fd.kvSeparator;
  a &fd.stringStorage;
  a &fd.intStorage;
}

template <typename Arch>
void Serialize(Arch &a, DictionarySpec &spec) {
  a &spec.fields;
  a &spec.aliasingSet;
  a &spec.indexColumn;
  a &spec.numIntStorage;
  a &spec.numStringStorage;
}

SERIALIZE_ENUM_CLASS(UnkMakerType, int);

SERIALIZE_ENUM_CLASS(FieldExpressionKind, int);

SERIALIZE_ENUM_CLASS(UnkFeatureType, int);

template <typename Arch>
void Serialize(Arch &a, UnkMakerFeature &o) {
  a &o.targetPlaceholder;
  a &o.featureType;
}

template <typename Arch>
void Serialize(Arch &a, UnkProcessorDescriptor &o) {
  a &o.index;
  a &o.name;
  a &o.type;
  a &o.patternRow;
  a &o.patternPtr;
  a &o.priority;
  a &o.charClass;
  a &o.features;
}

SERIALIZE_ENUM_CLASS(DicImportKind, int);

template <typename Arch>
void Serialize(Arch &a, DicImportDescriptor &o) {
  a &o.index;
  a &o.target;
  a &o.shift;
  a &o.name;
  a &o.kind;
  a &o.references;
  // we don't save data into model - no reason to do that
  // everything is already inside the dic
  // a &o.data;
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
void Serialize(Arch &a, ComputationFeatureDescriptor &o) {
  a &o.name;
  a &o.index;
  a &o.primitiveFeature;
  a &o.trueBranch;
  a &o.falseBranch;
}

template <typename Arch>
void Serialize(Arch &a, PatternFeatureDescriptor &o) {
  a &o.index;
  a &o.references;
}

template <typename Arch>
void Serialize(Arch &a, NgramFeatureDescriptor &o) {
  a &o.index;
  a &o.references;
}

template <typename Arch>
void Serialize(Arch &a, FeaturesSpec &o) {
  a &o.dictionary;
  a &o.primitive;
  a &o.computation;
  a &o.pattern;
  a &o.ngram;
  a &o.numPlaceholders;
  a &o.totalPrimitives;
  a &o.numDicFeatures;
  a &o.numDicData;
}

template <typename Arch>
void Serialize(Arch &a, TrainingField &o) {
  a &o.number;
  a &o.fieldIdx;
  a &o.dicIdx;
  a &o.weight;
}

template <typename Arch>
void Serialize(Arch &a, TrainingSpec &o) {
  a &o.fields;
  a &o.surfaceIdx;
}

template <typename Arch>
void Serialize(Arch &a, AnalysisSpec &spec) {
  a &spec.specMagic_;
  a &spec.specVersion_;
  a &spec.dictionary;
  a &spec.features;
  a &spec.unkCreators;
  a &spec.training;
  a &spec.specMagic2_;
}

}  // namespace spec
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SPEC_SER_H
