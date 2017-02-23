//
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "spec_dsl.h"
#include "util/flatset.h"

namespace jumanpp {
namespace core {
namespace spec {
namespace dsl {

Status ModelSpecBuilder::validateFields() const {
  util::FlatSet<i32> fieldIdx;
  util::FlatSet<StringPiece> fieldName;
  i32 indexCount = 0;
  for (auto& f : fields_) {
    JPP_RETURN_IF_ERROR(f.validate());
    if (fieldIdx.count(f.getCsvColumn()) == 0) {
      fieldIdx.insert(f.getCsvColumn());
    } else {
      return Status::InvalidParameter()
             << "coulmn indexes should be unique FAIL: " << f.name();
    }
    if (fieldName.count(f.name()) == 0) {
      fieldName.insert(f.name());
    } else {
      return Status::InvalidParameter() << "field names should be unique, "
                                        << f.name() << " is not";
    }
    if (f.isTrieIndex()) {
      indexCount += 1;
    }
  }
  if (indexCount != 1) {
    return Status::InvalidParameter()
           << "there should exist exactly one indexed field";
  }
  return Status::Ok();
}

Status ModelSpecBuilder::validate() const {
  JPP_RETURN_IF_ERROR(validateFields());
  return Status::Ok();
}

Status ModelSpecBuilder::build(AnalysisSpec* spec) const {
  JPP_RETURN_IF_ERROR(validate());
  makeFields(spec);
  return Status::Ok();
}

void ModelSpecBuilder::makeFields(AnalysisSpec* spec) const {
  auto& cols = spec->columns;
  for (size_t i = 0; i < fields_.size(); ++i) {
    auto& f = fields_[i];
    cols.emplace_back();  // make one with default constructor
    auto* col = &cols.back();
    col->index = (i32)i;
    f.fill(&cols.back());
    if (f.isTrieIndex()) {
      spec->indexColumn = (i32)i;
    }
  }
}

Status FieldBuilder::validate() const {
  if (name_.size() == 0) {
    return Status::InvalidParameter() << "field name should not be empty";
  }
  if (csvColumn_ <= 0) {
    return Status::InvalidParameter()
           << "column number must be a positive integer, field " << name_
           << " has " << csvColumn_;
  }
  if (columnType_ == ColumnType::Error) {
    return Status::InvalidParameter() << "coumn " << name_
                                      << " do not have type";
  }
  if (trieIndex_ && columnType_ != ColumnType::String) {
    return Status::InvalidParameter()
           << "only string field can be indexed, field " << this->name_
           << " is not one";
  }
  if (emptyValue_.size() > 0 && columnType_ == ColumnType::Int) {
    return Status::InvalidParameter()
           << "empty field does not make sense on int fields like "
           << this->name_;
  }

  return Status::Ok();
}

void FieldBuilder::fill(FieldDescriptor* descriptor) const {
  descriptor->position = csvColumn_;
  descriptor->columnType = columnType_;
  descriptor->emptyString = emptyValue_;
  descriptor->isTrieKey = trieIndex_;
  descriptor->name = name_.str();
}

Status FeatureBuilder::validate() const {
  if (type_ == FeatureType::Initial) {
    return Status::InvalidParameter() << "feature " << name_
                                      << " was not initialized";
  }
  if (type_ == FeatureType::Invalid) {
    return Status::InvalidParameter() << "feature " << name_
                                      << " was initialized more than one time";
  }
  if (type_ == FeatureType::Length && fields_.size() != 1) {
    return Status::InvalidState() << "feature " << name_
                                  << " can contain only one field reference";
  }

  return Status::Ok();
}
}  // dsl
}  // spec
}  // core
}  // jumanpp
