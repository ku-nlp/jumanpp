//
// Created by Arseny Tolmachev on 2017/03/04.
//

#include "spec_serialization.h"
#include "util/serialization.h"

namespace jumanpp {
namespace chars {
SERIALIZE_ENUM_CLASS(CharacterClass, i32);
}

namespace core {
namespace spec {


SERIALIZE_ENUM_CLASS(ColumnType, int);

template <typename Arch>
void Serialize(Arch &a, FieldDescriptor& fd) {
  a & fd.index;
  a & fd.position;
  a & fd.name;
  a & fd.isTrieKey;
  a & fd.columnType;
  a & fd.emptyString;
  a & fd.stringStorage;
  a & fd.intStorage;
}

template <typename Arch>
void Serialize(Arch& a, DictionarySpec& spec) {
  a & spec.columns;
  a & spec.indexColumn;
  a & spec.numIntStorage;
  a & spec.numStringStorage;
}

SERIALIZE_ENUM_CLASS(UnkMakerType, int);
SERIALIZE_ENUM_CLASS(FieldExpressionKind, int);

template <typename Arch>
void Serialize(Arch& a, FieldExpression& o) {
  a & o.fieldIndex;
  a & o.kind;
  a & o.intConstant;
  a & o.stringConstant;
}

template <typename Arch>
void Serialize(Arch& a, UnkMaker& o) {
  a & o.index;
  a & o.name;
  a & o.type;
  a & o.patternRow;
  a & o.charClass;
  a & o.featureExpressions;
  a & o.outputExpressions;
}

SERIALIZE_ENUM_CLASS(PrimitiveFeatureKind, int);

template <typename Arch>
void Serialize(Arch& a, PrimitiveFeatureDescriptor& o) {
  a & o.index;
  a & o.name;
  a & o.kind;
  a & o.references;
  a & o.matchData;
}

template <typename Arch>
void Serialize(Arch& a, MatchReference& o) {
  a & o.featureIdx;
  a & o.dicFieldIdx;
}

template <typename Arch>
void Serialize(Arch& a, ComputationFeatureDescriptor& o) {
  a & o.index;
  a & o.name;
  a & o.matchReference;
  a & o.matchData;
  a & o.trueBranch;
  a & o.falseBranch;
}

template <typename Arch>
void Serialize(Arch& a, PatternFeatureDescriptor& o) {
  a & o.index;
  a & o.references;
}

template <typename Arch>
void Serialize(Arch& a, FinalFeatureDescriptor& o) {
  a & o.index;
  a & o.references;
}

template <typename Arch>
void Serialize(Arch& a, FeaturesSpec& o) {
  a & o.primitive;
  a & o.computation;
  a & o.pattern;
  a & o.final;
  a & o.totalPrimitives;
}

template <typename Arch>
void Serialize(Arch& a, AnalysisSpec& spec) {
  a & spec.dictionary;
  a & spec.features;
  a & spec.unkCreators;
}

void saveSpec(const AnalysisSpec& spec, util::CodedBuffer* buf) {
  util::serialization::Saver s{buf};
  s.save(spec);
}

bool loadSpec(StringPiece data, AnalysisSpec*result) {
  util::serialization::Loader l {data};
  return l.load(result);
}

} // spec
} // core
} // jumanpp