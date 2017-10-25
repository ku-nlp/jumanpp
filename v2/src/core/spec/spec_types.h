//
// Created by Arseny Tolmachev on 2017/02/23.
//

#ifndef JUMANPP_SPEC_TYPES_H
#define JUMANPP_SPEC_TYPES_H

#include <limits>
#include <vector>
#include "util/characters.h"
#include "util/string_piece.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace spec {

enum class ColumnType { String, Int, StringList, StringKVList, Error };

std::ostream& operator<<(std::ostream& o, ColumnType ct);

struct FieldDescriptor {
  i32 index = -1;
  i32 position;
  std::string name;
  bool isTrieKey = false;
  ColumnType columnType = ColumnType::Error;
  std::string emptyString;
  std::string listSeparator;
  std::string kvSeparator;
  i32 stringStorage = -1;
  i32 intStorage = -1;
};

struct DictionarySpec {
  std::vector<FieldDescriptor> columns;
  i32 indexColumn = -1;
  i32 numIntStorage = -1;
  i32 numStringStorage = -1;
};

enum class UnkMakerType {
  Invalid,
  Single,
  Chunking,
  Onomatopoeia,
  Numeric,
  Normalize
};

enum class FieldExpressionKind {
  Invalid,
  ReplaceWithMatch,
  ReplaceString,
  ReplaceInt,
  AppendString
};

static constexpr i32 InvalidIntConstant = std::numeric_limits<i32>::min();

struct FieldExpression {
  i32 fieldIndex = -1;
  FieldExpressionKind kind = FieldExpressionKind::Invalid;
  i32 intConstant = InvalidIntConstant;
  std::string stringConstant;
};

enum class UnkFeatureType { NotPrefixOfDicWord };

struct UnkMakerFeature {
  UnkFeatureType type;
  i32 reference;
};

struct UnkMaker {
  i32 index = -1;
  std::string name;
  UnkMakerType type = UnkMakerType::Invalid;
  // 1-based
  i32 patternRow = -1;
  i32 priority = 0;
  chars::CharacterClass charClass = chars::CharacterClass::FAMILY_OTHERS;
  std::vector<UnkMakerFeature> features;
  std::vector<FieldExpression> outputExpressions;
};

enum class PrimitiveFeatureKind {
  Invalid,
  Copy,
  MatchDic,
  MatchListElem,
  Provided,
  Length,
  CodepointSize,
  MatchKey,
  CodepointType,
  Codepoint
};

struct PrimitiveFeatureDescriptor {
  i32 index;
  std::string name;
  PrimitiveFeatureKind kind;
  std::vector<i32> references;
  std::vector<std::string> matchData;
};

struct MatchReference {
  i32 featureIdx;
  i32 dicFieldIdx;
};

struct ComputationFeatureDescriptor {
  i32 index;
  std::string name;
  std::vector<MatchReference> matchReference;
  std::vector<std::string> matchData;
  std::vector<i32> trueBranch;
  std::vector<i32> falseBranch;
};

struct PatternFeatureDescriptor {
  i32 index;
  std::vector<i32> references;
};

struct FinalFeatureDescriptor {
  i32 index;
  std::vector<i32> references;
};

struct FeaturesSpec {
  std::vector<PrimitiveFeatureDescriptor> primitive;
  std::vector<ComputationFeatureDescriptor> computation;
  std::vector<PatternFeatureDescriptor> pattern;
  std::vector<FinalFeatureDescriptor> final;
  i32 totalPrimitives = -1;
};

struct TrainingField {
  i32 number;
  i32 fieldIdx;
  float weight;
};

struct TrainingSpec {
  i32 surfaceIdx;
  std::vector<TrainingField> fields;
};

struct AnalysisSpec {
  DictionarySpec dictionary;
  FeaturesSpec features;
  std::vector<UnkMaker> unkCreators;
  TrainingSpec training;
};

}  // namespace spec
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SPEC_TYPES_H
