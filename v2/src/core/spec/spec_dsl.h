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
  virtual ~DslOpBase() {}
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

struct FieldExpression {
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

class FieldBuilder : public DslOpBase {
  util::memory::ManagedAllocatorCore* alloc_;
  i32 csvColumn_;
  StringPiece name_;
  ColumnType columnType_ = ColumnType::Error;
  bool trieIndex_ = false;
  StringPiece emptyValue_;

  FieldBuilder() {}

 public:
  FieldBuilder(util::memory::ManagedAllocatorCore* alloc, i32 csvColumn_,
               const StringPiece& name_)
      : alloc_{alloc}, csvColumn_(csvColumn_), name_(name_) {}

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

  FieldBuilder& trieIndex() {
    trieIndex_ = true;
    return *this;
  }

  FieldBuilder& emptyValue(StringPiece data) {
    emptyValue_ = data;
    return *this;
  }

  FieldExpression value() const {
    return FieldExpression{name_, TransformType::Value, jumanpp::EMPTY_SP, 0};
  }

  FieldExpression replaceWith(StringPiece value) const {
    return FieldExpression{name_, TransformType::ReplaceString, value, 0};
  }

  FieldExpression replaceWith(i32 value) const {
    return FieldExpression{name_, TransformType::ReplaceInt, jumanpp::EMPTY_SP,
                           value};
  }

  FieldExpression append(StringPiece value) const {
    return FieldExpression{name_, TransformType::AppendString, value, 0};
  }

  i32 getCsvColumn() const { return csvColumn_; }

  StringPiece name() const { return name_; }

  ColumnType getColumnType() const { return columnType_; }

  bool isTrieIndex() const { return trieIndex_; }

  virtual Status validate() const override;

  operator FieldReference() const { return FieldReference{name()}; }

  void fill(FieldDescriptor* descriptor) const;
};

enum class FeatureType {
  Initial,
  Invalid,
  MatchValue,
  MatchCsv,
  Length,
  Placeholder
};

class FeatureBuilder : DslOpBase {
  bool handled = false;
  StringPiece name_;
  FeatureType type_ = FeatureType::Initial;
  StringPiece matchData_;
  util::InlinedVector<StringPiece, 4> fields_;
  util::InlinedVector<FieldExpression, 8> trueTransforms_;
  util::InlinedVector<FieldExpression, 8> falseTransforms_;

  void changeType(FeatureType target) {
    if (type_ == FeatureType::Initial) {
      type_ = target;
    } else {
      type_ = FeatureType::Invalid;
    }
  }

  friend class ModelSpecBuilder;

 public:
  FeatureBuilder(const StringPiece& name_) : name_(name_) {}

  StringPiece name() const { return name_; }

  FeatureBuilder& placeholder() {
    changeType(FeatureType::Placeholder);
    return *this;
  }

  FeatureBuilder& length(FieldBuilder& field) {
    fields_.push_back(field.name());
    changeType(FeatureType::Length);
    return *this;
  }

  FeatureBuilder& matchValue(FieldReference field, StringPiece value) {
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

  FeatureBuilder& ifTrue(std::initializer_list<FieldExpression> transforms) {
    for (auto&& o : transforms) {
      trueTransforms_.emplace_back(o);
    }
    return *this;
  }

  FeatureBuilder& ifFalse(std::initializer_list<FieldExpression> transforms) {
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
  FeatureRef(const FieldBuilder& fld) : name_{fld.name()} {}
  FeatureRef(const FeatureBuilder& ft) : name_{ft.name()} {}
  StringPiece name() const { return name_; }
};

class FeatureCombinator {
  util::memory::ManagedVector<util::memory::ManagedVector<FeatureRef>> data;
  friend class ModelSpecBuilder;

 public:
  FeatureCombinator(util::memory::ManagedAllocatorCore* alloc) : data{alloc} {}
};

class ModelSpecBuilder : public DslOpBase {
  util::memory::Manager memmgr_;
  std::unique_ptr<util::memory::ManagedAllocatorCore> alloc_;
  util::memory::ManagedVector<FieldBuilder*> fields_;
  util::memory::ManagedVector<FeatureBuilder*> features_;
  util::memory::ManagedVector<FeatureCombinator*> combinators_;
  mutable i32 currentFeature_ = 0;

  void makeFields(AnalysisSpec* spec) const;
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

 public:
  ModelSpecBuilder(size_t page_size = 16 * 1024)
      : memmgr_{page_size},
        alloc_{memmgr_.core()},
        fields_{alloc_.get()},
        features_{alloc_.get()},
        combinators_{alloc_.get()} {}

  FieldBuilder& field(i32 csvColumn, StringPiece name) {
    auto ptr = alloc_->make<FieldBuilder>(alloc_.get(), csvColumn, name);
    fields_.emplace_back(ptr);
    return *ptr;
  }

  FeatureBuilder& feature(StringPiece name) {
    auto ptr = alloc_->make<FeatureBuilder>(name);
    features_.emplace_back(ptr);
    return *ptr;
  }

  void unigram(const std::initializer_list<FeatureRef>& f1) {
    auto cmb = alloc_->make<FeatureCombinator>(alloc_.get());
    auto& data = cmb->data;
    data.emplace_back(f1, alloc_.get());
    combinators_.emplace_back(cmb);
  }

  Status validateFields() const;
  Status validateNames() const;
  Status validateFeatures() const;
  virtual Status validate() const override;
  Status build(AnalysisSpec* spec) const;
};
}
}  // spec
}  // core
}  // jumanpp

#endif  // JUMANPP_SPEC_DSL_H
