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

enum class FieldType { String, Int, StringList, StringKVList, Error };

std::ostream& operator<<(std::ostream& o, FieldType ct);

constexpr static i32 InvalidInt = std::numeric_limits<i32>::min();

constexpr static u32 SpecMagic = 0xfeed0000;
constexpr static u32 SpecFormatVersion = 3;

struct FieldDescriptor {
  /**
   * # of field in definition. All spec field references use this number.
   */
  i32 specIndex = InvalidInt;
  /**
   * # of field inside CSV (1-based)
   * A value of 0 means that the field was auto-generated,
   * stores some internal data and do NOT exist in csv dictionary file
   */
  i32 position = InvalidInt;
  /**
   * # of field inside the compiled dictionary,
   * positive values are for feature fields,
   * negative (one-compelement!!!!) values are for data fields.
   */
  i32 dicIndex = InvalidInt;
  std::string name;
  bool isTrieKey = false;
  FieldType fieldType = FieldType::Error;
  std::string emptyString;
  std::string listSeparator;
  std::string kvSeparator;
  i32 stringStorage = InvalidInt;
  i32 intStorage = InvalidInt;
  i32 alignment = 0;
};

struct DictionarySpec {
  std::vector<FieldDescriptor> fields;
  std::vector<i32> aliasingSet;
  i32 indexColumn = -1;
  i32 numIntStorage = -1;
  i32 numStringStorage = -1;

  const FieldDescriptor& forDicIdx(i32 dicIdx) const {
    for (auto& f : fields) {
      if (f.dicIndex == dicIdx) {
        return f;
      }
    }
    JPP_DCHECK_NOT("could not find a dic field for index");
    std::terminate();
  }
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

enum class UnkFeatureType { NotPrefixOfDic, NormalizedActions };

struct UnkMakerFeature {
  i32 targetPlaceholder;
  i32 targetFeature;
  UnkFeatureType featureType;
};

struct UnkProcessorDescriptor {
  i32 index = -1;
  std::string name;
  UnkMakerType type = UnkMakerType::Invalid;
  // 1-based
  i32 patternRow = -1;
  i32 patternPtr = InvalidInt;
  i32 priority = 0;
  chars::CharacterClass charClass = chars::CharacterClass::FAMILY_OTHERS;
  std::vector<UnkMakerFeature> features;
  std::vector<i32> replaceFields;
};

enum class PrimitiveFeatureKind {
  Invalid,
  Copy,
  SingleBit,
  Provided,
  ByteLength,
  CodepointSize,
  SurfaceCodepointSize,
  CodepointType,
  Codepoint,
};

enum class DicImportKind {
  Invalid,
  ImportAsFeature,
  MatchListKey,
  MatchFields,
  ImportAsData = 1000
};

struct DicImportDescriptor {
  i32 index = InvalidInt;
  i32 target = InvalidInt;
  i32 shift = InvalidInt;
  std::string name;
  DicImportKind kind;
  std::vector<i32> references;
  std::vector<std::string> data;
};

struct PrimitiveFeatureDescriptor {
  i32 index = InvalidInt;
  std::string name;
  PrimitiveFeatureKind kind;
  std::vector<i32> references;
  std::vector<std::string> matchData;
};

struct ComputationFeatureDescriptor {
  std::string name;
  i32 index = InvalidInt;
  i32 primitiveFeature = InvalidInt;
  std::vector<i32> trueBranch;
  std::vector<i32> falseBranch;
};

struct PatternFeatureDescriptor {
  i32 index;
  i32 usage;
  std::vector<i32> references;
};

struct NgramFeatureDescriptor {
  i32 index = InvalidInt;
  std::vector<i32> references;
};

struct FeaturesSpec {
  std::vector<DicImportDescriptor> dictionary;
  std::vector<PrimitiveFeatureDescriptor> primitive;
  std::vector<ComputationFeatureDescriptor> computation;
  std::vector<PatternFeatureDescriptor> pattern;
  std::vector<NgramFeatureDescriptor> ngram;
  i32 numPlaceholders = 0;
  i32 totalPrimitives = InvalidInt;
  i32 numDicFeatures = InvalidInt;
  i32 numDicData = InvalidInt;
  i32 numUniOnlyPats = InvalidInt;
};

struct TrainingField {
  i32 number;
  i32 fieldIdx;
  i32 dicIdx;
  float weight;
};

struct AllowedUnkField {
  i32 targetField;
  i32 sourceField;
  std::string sourceKey;
};

struct TrainingSpec {
  i32 surfaceIdx;
  std::vector<TrainingField> fields;
  std::vector<AllowedUnkField> allowedUnk;
};

struct AnalysisSpec {
  u32 specMagic_ = SpecMagic;
  u32 specVersion_ = SpecFormatVersion;
  DictionarySpec dictionary;
  FeaturesSpec features;
  std::vector<UnkProcessorDescriptor> unkCreators;
  TrainingSpec training;
  u32 specMagic2_ = SpecMagic;

  Status validate() const;
};

}  // namespace spec
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SPEC_TYPES_H
