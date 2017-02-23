//
// Created by Arseny Tolmachev on 2017/02/23.
//

#ifndef JUMANPP_SPEC_DSL_H
#define JUMANPP_SPEC_DSL_H

#include <vector>
#include "core/dic_spec.h"
#include "util/inlined_vector.h"
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
class UniFeatureBuilder;
class BiFeatureBuilder;
class TriFeatureBuilder;

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
  i32 csvColumn_;
  StringPiece name_;
  dic::ColumnType columnType_ = dic::ColumnType::Error;
  bool trieIndex_ = false;
  StringPiece emptyValue_;

  FieldBuilder() {}

 public:
  FieldBuilder(i32 csvColumn_, const StringPiece& name_)
      : csvColumn_(csvColumn_), name_(name_) {}

  FieldBuilder& strings() {
    columnType_ = dic::ColumnType::String;
    return *this;
  }

  FieldBuilder& stringLists() {
    columnType_ = dic::ColumnType::StringList;
    return *this;
  }

  FieldBuilder& integers() {
    columnType_ = dic::ColumnType::Int;
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
    return FieldExpression{name_, TransformType::Value, "", 0};
  }

  FieldExpression replaceWith(StringPiece value) const {
    return FieldExpression{name_, TransformType::ReplaceString, value, 0};
  }

  FieldExpression replaceWith(i32 value) const {
    return FieldExpression{name_, TransformType::ReplaceInt, "", value};
  }

  FieldExpression append(StringPiece value) const {
    return FieldExpression{name_, TransformType::AppendString, value, 0};
  }

  i32 getCsvColumn() const { return csvColumn_; }

  const StringPiece& name() const { return name_; }

  dic::ColumnType getColumnType_() const { return columnType_; }

  bool isTrieIndex_() const { return trieIndex_; }

  const StringPiece& getEmptyValue_() const { return emptyValue_; }

  virtual Status validate() const override;

  operator FieldReference() const { return FieldReference{name()}; }
};

enum class FeatureType { Initial, Invalid, MatchValue, MatchCsv, Length };

class FeatureBuilder : DslOpBase {
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

 public:
  FeatureBuilder(const StringPiece& name_) : name_(name_) {}

  StringPiece name() const { return name_; }

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

  Status validate() const override {
    if (type_ == FeatureType::Initial) {
      return Status::InvalidParameter() << "feature " << name_
                                        << " was not initialized";
    }
    if (type_ == FeatureType::Invalid) {
      return Status::InvalidParameter()
             << "feature " << name_ << " was initialized more than one time";
    }
    if (type_ == FeatureType::Length && fields_.size() != 1) {
      return Status::InvalidState() << "feature " << name_
                                    << " can contain only one field reference";
    }

    return Status::Ok();
  }
};

class ModelSpecBuilder : public DslOpBase {
  std::vector<FieldBuilder> fields_;
  std::vector<FeatureBuilder> features_;

 public:
  FieldBuilder& field(i32 csvColumn, StringPiece name) {
    fields_.emplace_back(csvColumn, name);
    return fields_.back();
  }

  FeatureBuilder& feature(StringPiece name) {
    features_.emplace_back(name);
    return features_.back();
  }

  Status validateFields() const;
  virtual Status validate() const override;
};
}
}  // spec
}  // core
}  // jumanpp

#endif  // JUMANPP_SPEC_DSL_H
