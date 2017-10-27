//
// Created by Arseny Tolmachev on 2017/02/23.
//

#ifndef JUMANPP_SPEC_DSL_H
#define JUMANPP_SPEC_DSL_H

#include <util/array_slice.h>
#include <vector>
#include "core/spec/spec_types.h"
#include "util/flatset.h"
#include "util/inlined_vector.h"
#include "util/memory.hpp"
#include "util/status.hpp"
#include "util/string_piece.h"

namespace jumanpp {
namespace core {
namespace spec {
namespace dsl {

class DslOpBase {
 public:
  virtual ~DslOpBase() = default;
  virtual Status validate() const = 0;
};

class FieldBuilder;
class FeatureBuilder;
class UnkWordBuilder;
class FeatureCombinator;

enum class TransformType {
  Invalid,
  Value,
  ReplaceString,
  ReplaceInt,
  AppendString
};

struct FieldExpressionBldr {
  StringPiece fieldName;
  TransformType type = TransformType::Invalid;
  StringPiece transformString;
  int transformInt = std::numeric_limits<i32>::min();
};

class FieldReference {
  StringPiece name_;
  FieldReference(StringPiece name) : name_{name} {}

  friend class FieldBuilder;

 public:
  StringPiece name() const { return name_; }
};

class StorageAssigner;

class FieldBuilder : public DslOpBase {
  i32 csvColumn_;
  StringPiece name_;
  ColumnType columnType_ = ColumnType::Error;
  bool trieIndex_ = false;
  std::string emptyValue_;
  StringPiece stringStorage_;
  std::string fieldSeparator_ = " ";
  std::string kvSeparator_ = ":";

  FieldBuilder() {}

 public:
  FieldBuilder(i32 csvColumn_, const StringPiece& name_)
      : csvColumn_(csvColumn_), name_(name_) {}

  FieldBuilder& strings() {
    columnType_ = ColumnType::String;
    return *this;
  }

  FieldBuilder& stringLists() {
    columnType_ = ColumnType::StringList;
    return *this;
  }

  FieldBuilder& integers() {
    columnType_ = ColumnType::Int;
    return *this;
  }

  FieldBuilder& kvLists() {
    columnType_ = ColumnType::StringKVList;
    return *this;
  }

  FieldBuilder& trieIndex() {
    trieIndex_ = true;
    return *this;
  }

  FieldBuilder& emptyValue(StringPiece data) {
    emptyValue_ = data.str();
    return *this;
  }

  FieldBuilder& stringStorage(FieldReference ref) {
    stringStorage_ = ref.name();
    return *this;
  }

  FieldBuilder& listSeparator(StringPiece sep) {
    this->fieldSeparator_ = sep.str();
    return *this;
  }

  FieldBuilder& kvSeparator(StringPiece sep) {
    this->kvSeparator_ = sep.str();
    return *this;
  }

  FieldExpressionBldr value() const {
    return FieldExpressionBldr{name_, TransformType::Value, jumanpp::EMPTY_SP,
                               0};
  }

  operator FieldExpressionBldr() const { return value(); }

  FieldExpressionBldr replaceWith(StringPiece value) const {
    return FieldExpressionBldr{name_, TransformType::ReplaceString, value, 0};
  }

  FieldExpressionBldr replaceWith(i32 value) const {
    return FieldExpressionBldr{name_, TransformType::ReplaceInt,
                               jumanpp::EMPTY_SP, value};
  }

  FieldExpressionBldr append(StringPiece value) const {
    return FieldExpressionBldr{name_, TransformType::AppendString, value, 0};
  }

  i32 getCsvColumn() const { return csvColumn_; }

  StringPiece name() const { return name_; }

  ColumnType getColumnType() const { return columnType_; }

  bool isTrieIndex() const { return trieIndex_; }

  virtual Status validate() const override;

  operator FieldReference() const { return FieldReference{name()}; }

  Status fill(FieldDescriptor* descriptor, StorageAssigner* sa) const;
};

enum class FeatureType {
  Initial,
  Invalid,
  MatchValue,
  MatchCsv,
  Length,
  CodepointSize,
  Placeholder,
  CodepointType,
  Codepoint
};

class FeatureBuilder : DslOpBase {
  bool handled = false;
  StringPiece name_;
  FeatureType type_ = FeatureType::Initial;
  StringPiece matchData_;
  util::InlinedVector<StringPiece, 4> fields_;
  util::InlinedVector<FieldExpressionBldr, 8> trueTransforms_;
  util::InlinedVector<FieldExpressionBldr, 8> falseTransforms_;
  i32 intParam_;

  void changeType(FeatureType target) {
    if (type_ == FeatureType::Initial) {
      type_ = target;
    } else {
      type_ = FeatureType::Invalid;
    }
  }

  friend class ModelSpecBuilder;

 public:
  FeatureBuilder(const FeatureBuilder&) = delete;
  FeatureBuilder(const StringPiece& name_) : name_(name_) {}

  StringPiece name() const { return name_; }

  FeatureBuilder& placeholder() {
    changeType(FeatureType::Placeholder);
    return *this;
  }

  FeatureBuilder& numCodepoints(FieldBuilder& field) {
    fields_.push_back(field.name());
    changeType(FeatureType::CodepointSize);
    return *this;
  }

  FeatureBuilder& length(FieldBuilder& field) {
    fields_.push_back(field.name());
    changeType(FeatureType::Length);
    return *this;
  }

  FeatureBuilder& matchData(FieldReference field, StringPiece value) {
    fields_.push_back(field.name());
    matchData_ = value;
    changeType(FeatureType::MatchValue);
    return *this;
  }

  FeatureBuilder& matchAnyRowOfCsv(
      StringPiece csv, std::initializer_list<FieldReference> fields) {
    for (auto& fld : fields) {
      fields_.push_back(fld.name());
    }
    matchData_ = csv;
    changeType(FeatureType::MatchCsv);
    return *this;
  }

  FeatureBuilder& ifTrue(
      std::initializer_list<FieldExpressionBldr> transforms) {
    for (auto&& o : transforms) {
      trueTransforms_.emplace_back(o);
    }
    return *this;
  }

  FeatureBuilder& codepointType(i32 offset) {
    intParam_ = offset;
    changeType(FeatureType::CodepointType);
    return *this;
  }

  FeatureBuilder& codepoint(i32 offset) {
    intParam_ = offset;
    changeType(FeatureType::Codepoint);
    return *this;
  }

  FeatureBuilder& ifFalse(
      std::initializer_list<FieldExpressionBldr> transforms) {
    for (auto&& o : transforms) {
      falseTransforms_.emplace_back(o);
    }
    return *this;
  }

  Status validate() const override;
};

class FeatureRef {
  StringPiece name_;

 public:
  FeatureRef() = default;
  FeatureRef(const FieldBuilder& fld) : name_{fld.name()} {}
  FeatureRef(const FeatureBuilder& ft) : name_{ft.name()} {}
  FeatureRef(const FeatureRef& o) = default;
  StringPiece name() const { return name_; }
};

class FeatureCombinator {
  util::memory::ManagedVector<util::memory::ManagedVector<FeatureRef>> data;
  friend class ModelSpecBuilder;

 public:
  FeatureCombinator(util::memory::PoolAlloc* alloc) : data{alloc} {}
};

struct UnkProcFeature {
  UnkFeatureType type;
  FeatureRef ref;
};

class UnkProcBuilder : public DslOpBase {
  StringPiece name_;
  UnkMakerType type_ = UnkMakerType::Invalid;
  chars::CharacterClass charClass_ = chars::CharacterClass::FAMILY_OTHERS;
  i32 pattern_ = -1;
  i32 priority_ = 0;
  std::vector<UnkProcFeature> surfaceFeatures_;
  std::vector<FieldExpressionBldr> output_;

  friend class ModelSpecBuilder;

 public:
  UnkProcBuilder(StringPiece name, i32 pattern)
      : name_{name}, pattern_{pattern} {}

  UnkProcBuilder(const UnkProcBuilder&) = delete;

  UnkProcBuilder& single(chars::CharacterClass charClass) {
    type_ = UnkMakerType::Single;
    charClass_ = charClass;
    return *this;
  }

  UnkProcBuilder& chunking(chars::CharacterClass charClass) {
    type_ = UnkMakerType::Chunking;
    charClass_ = charClass;
    return *this;
  }

  UnkProcBuilder& onomatopoeia(chars::CharacterClass charClass) {
    type_ = UnkMakerType::Onomatopoeia;
    charClass_ = charClass;
    return *this;
  }

  UnkProcBuilder& numeric(chars::CharacterClass charClass) {
    type_ = UnkMakerType::Numeric;
    charClass_ = charClass;
    return *this;
  }

  UnkProcBuilder& normalize() {
    type_ = UnkMakerType::Normalize;
    return *this;
  }

  UnkProcBuilder& notPrefixOfDicFeature(FeatureRef ref) {
    UnkProcFeature upf{UnkFeatureType::NotPrefixOfDicWord, ref};
    surfaceFeatures_.emplace_back(upf);
    return *this;
  }

  UnkProcBuilder& outputTo(std::initializer_list<FieldExpressionBldr> field) {
    for (auto& x : field) {
      output_.push_back(x);
    }
    return *this;
  }

  UnkProcBuilder& lowPriority() {
    priority_ = 1;
    return *this;
  }

  StringPiece name() const { return name_; }

  Status validate() const override;
};

class TrainExampleSpec {
  std::vector<std::pair<FieldReference, float>> fields;
  friend class ModelSpecBuilder;

 public:
  TrainExampleSpec& field(FieldReference ref, float weight) {
    fields.push_back({ref, weight});
    return *this;
  }
};

class ModelSpecBuilder : public DslOpBase {
  util::memory::Manager memmgr_;
  std::unique_ptr<util::memory::PoolAlloc> alloc_;
  util::memory::ManagedVector<FieldBuilder*> fields_;
  util::memory::ManagedVector<FeatureBuilder*> features_;
  util::memory::ManagedVector<FeatureCombinator*> combinators_;
  util::memory::ManagedVector<UnkProcBuilder*> unks_;
  util::memory::ManagedPtr<TrainExampleSpec> train_;
  mutable i32 currentFeature_ = 0;

  Status makeFields(AnalysisSpec* spec) const;
  Status makeFeatures(AnalysisSpec* spec) const;
  void collectUsedNames(util::FlatSet<StringPiece>* names) const;
  void createCopyFeatures(
      const util::ArraySlice<FieldDescriptor>& fields,
      const util::FlatSet<StringPiece>& names,
      std::vector<PrimitiveFeatureDescriptor>* result) const;
  Status createRemainingPrimitiveFeatures(
      const util::ArraySlice<FieldDescriptor>& fields,
      std::vector<PrimitiveFeatureDescriptor>* result) const;
  Status checkNoFeatureIsLeft() const;
  Status createPatternsAndFinalFeatures(FeaturesSpec* spec) const;
  Status createComputeFeatures(FeaturesSpec* fspec) const;
  Status createUnkProcessors(AnalysisSpec* spec) const;
  Status createTrainSpec(AnalysisSpec* spec) const;

 public:
  ModelSpecBuilder(size_t page_size = 16 * 1024)
      : memmgr_{page_size},
        alloc_{memmgr_.core()},
        fields_{alloc_.get()},
        features_{alloc_.get()},
        combinators_{alloc_.get()},
        unks_{alloc_.get()} {}

  ModelSpecBuilder(const ModelSpecBuilder& o) = delete;

  FieldBuilder& field(i32 csvColumn, StringPiece name) {
    auto ptr = alloc_->make<FieldBuilder>(csvColumn, name);
    fields_.emplace_back(ptr);
    return *ptr;
  }

  FeatureBuilder& feature(StringPiece name) {
    auto ptr = alloc_->make<FeatureBuilder>(name);
    features_.emplace_back(ptr);
    return *ptr;
  }

  UnkProcBuilder& unk(StringPiece name, i32 pattern) {
    auto ptr = alloc_->make<UnkProcBuilder>(name, pattern);
    unks_.emplace_back(ptr);
    return *ptr;
  }

  void unigram(const std::initializer_list<FeatureRef>& t0) {
    auto cmb = alloc_->make<FeatureCombinator>(alloc_.get());
    auto& data = cmb->data;
    data.emplace_back(t0, alloc_.get());
    combinators_.emplace_back(cmb);
  }

  void bigram(const std::initializer_list<FeatureRef>& t1,
              const std::initializer_list<FeatureRef>& t0) {
    auto cmb = alloc_->make<FeatureCombinator>(alloc_.get());
    auto& data = cmb->data;
    data.emplace_back(t0, alloc_.get());
    data.emplace_back(t1, alloc_.get());
    combinators_.emplace_back(cmb);
  }

  void trigram(const std::initializer_list<FeatureRef>& t2,
               const std::initializer_list<FeatureRef>& t1,
               const std::initializer_list<FeatureRef>& t0) {
    auto cmb = alloc_->make<FeatureCombinator>(alloc_.get());
    auto& data = cmb->data;
    data.emplace_back(t0, alloc_.get());
    data.emplace_back(t1, alloc_.get());
    data.emplace_back(t2, alloc_.get());
    combinators_.emplace_back(cmb);
  }

  TrainExampleSpec& train() {
    train_ = alloc_->make_unique<TrainExampleSpec>();
    return *train_;
  }

  Status validateFields() const;
  Status validateNames() const;
  Status validateFeatures() const;
  Status validateUnks() const;
  virtual Status validate() const override;
  Status build(AnalysisSpec* spec) const;
};
}  // namespace dsl
}  // namespace spec
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SPEC_DSL_H
