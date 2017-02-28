//
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "spec_dsl.h"
#include "util/flatset.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace spec {
namespace dsl {

Status ModelSpecBuilder::validateFields() const {
  util::FlatSet<i32> fieldIdx;
  i32 indexCount = 0;
  for (auto fp : fields_) {
    auto& f = *fp;
    JPP_RETURN_IF_ERROR(f.validate());
    if (fieldIdx.count(f.getCsvColumn()) == 0) {
      fieldIdx.insert(f.getCsvColumn());
    } else {
      return Status::InvalidParameter()
             << "coulmn indexes should be unique FAIL: " << f.name();
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
  JPP_RETURN_IF_ERROR(validateNames());
  JPP_RETURN_IF_ERROR(validateFeatures());
  return Status::Ok();
}

Status ModelSpecBuilder::validateNames() const {
  util::FlatSet<StringPiece> current;
  for (auto fld : fields_) {
    if (current.count(fld->name()) != 0) {
      return Status::InvalidParameter() << "field " << fld->name()
                                        << " has non-unique name";
    }
    current.insert(fld->name());
  }

  for (auto feature : features_) {
    if (current.count(feature->name()) != 0) {
      return Status::InvalidParameter() << "feature " << feature->name()
                                        << " has non-unique name";
    }
  }

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
    f->fill(&cols.back());
    if (f->isTrieIndex()) {
      spec->indexColumn = (i32)i;
    }
  }
}

Status ModelSpecBuilder::validateFeatures() const {
  for (auto f : features_) {
    JPP_RETURN_IF_ERROR(f->validate());
  }

  return Status::Ok();
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

  if ((trueTransforms_.size() != 0) ^ (falseTransforms_.size() != 0)) {
    return Status::InvalidParameter()
           << name_
           << ": matching features must ether have both branches or have none";
  }

  if (trueTransforms_.size() != 0 && falseTransforms_.size() != 0) {
    if (!util::contains({FeatureType::MatchCsv, FeatureType::MatchValue},
                        type_)) {
      return Status::InvalidParameter()
             << name_ << ": only matching features can have branches";
    }
  }

  return Status::Ok();
}
}  // dsl
}  // spec
}  // core
}  // jumanpp
