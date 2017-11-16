//
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "core/spec/spec_dsl.h"
#include <util/logging.hpp>
#include "core/spec/spec_compiler.h"
#include "util/csv_reader.h"
#include "util/flatmap.h"
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
    JPP_RIE_MSG(f.validate(), "field name: " << f.name());
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
  JPP_RETURN_IF_ERROR(validateUnks());
  return Status::Ok();
}

Status ModelSpecBuilder::validateNames() const {
  util::FlatSet<StringPiece> current;
  for (auto fld : fields_) {
    if (current.count(fld->name()) != 0) {
      return Status::InvalidParameter()
             << "field " << fld->name() << " has non-unique name";
    }
    current.insert(fld->name());
  }

  for (auto feature : features_) {
    if (current.count(feature->name()) != 0) {
      return Status::InvalidParameter()
             << "feature " << feature->name() << " has non-unique name";
    }
  }

  return Status::Ok();
}

Status ModelSpecBuilder::build(AnalysisSpec* spec) const {
  JPP_RETURN_IF_ERROR(validate());
  SpecCompiler compiler{*this, spec};
  JPP_RETURN_IF_ERROR(compiler.build());
  JPP_RETURN_IF_ERROR(checkNoFeatureIsLeft());

  for (auto& f : features_) {
    if (f->notUsed) {
      LOG_WARN() << "the feature " << f->name() << " was not used";
    }
  }
  return Status::Ok();
}

Status ModelSpecBuilder::validateFeatures() const {
  for (auto f : features_) {
    f->handled = false;
    JPP_RIE_MSG(f->validate(), "for feature: " << f->name());
  }

  return Status::Ok();
}

Status ModelSpecBuilder::checkNoFeatureIsLeft() const {
  for (auto f : features_) {
    if (!f->handled) {
      return Status::InvalidState() << f->name() << ": feature was not handled";
    }
  }
  return Status::Ok();
}

Status ModelSpecBuilder::validateUnks() const {
  for (auto unk : unks_) {
    JPP_RETURN_IF_ERROR(unk->validate());
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
  if (columnType_ == FieldType::Error) {
    return Status::InvalidParameter()
           << "coumn " << name_ << " do not have type";
  }
  if (trieIndex_ && columnType_ != FieldType::String) {
    return Status::InvalidParameter()
           << "only string field can be indexed, field " << this->name_
           << " is not one";
  }
  if (emptyValue_.size() > 0 && columnType_ == FieldType::Int) {
    return Status::InvalidParameter()
           << "empty field does not make sense on int fields like "
           << this->name_;
  }

  if (stringStorage_.size() > 0) {
    if (!util::contains(
            {FieldType::StringList, FieldType::String, FieldType::StringKVList},
            columnType_)) {
      return Status::InvalidParameter() << "string storage can be specified "
                                           "only for string or stringList "
                                           "typed columns";
    }

    if (alignment_ != 0) {
      return JPPS_INVALID_PARAMETER
             << "either alignment or string sotrage can be specified";
    }
  }

  return Status::Ok();
}

Status FieldBuilder::fill(FieldDescriptor* descriptor,
                          StringPiece* stringStorName) const {
  descriptor->position = csvColumn_;
  descriptor->fieldType = columnType_;
  descriptor->emptyString = emptyValue_;
  descriptor->isTrieKey = trieIndex_;
  descriptor->name = name_.str();
  descriptor->listSeparator = fieldSeparator_;
  descriptor->kvSeparator = kvSeparator_;
  descriptor->alignment = alignment_;
  *stringStorName = stringStorage_.size() == 0 ? name() : stringStorage_;
  return Status::Ok();
}

Status FeatureBuilder::validate() const {
  if (type_ == FeatureType::Initial) {
    return JPPS_INVALID_PARAMETER << "feature " << name_
                                  << " was not initialized";
  }

  if (type_ == FeatureType::Invalid) {
    return JPPS_INVALID_PARAMETER << "feature " << name_
                                  << " was initialized more than one time";
  }

  if (type_ == FeatureType::ByteLength && fields_.size() != 1) {
    return JPPS_INVALID_PARAMETER << "feature " << name_
                                  << " can contain only one field reference";
  }

  if (type_ == FeatureType::CodepointSize && fields_.size() != 1) {
    return JPPS_INVALID_PARAMETER << "feature " << name_
                                  << " can contain only one field reference";
  }

  if ((trueTransforms_.size() != 0) ^ (falseTransforms_.size() != 0)) {
    return JPPS_INVALID_PARAMETER
           << name_
           << ": matching features must ether have both branches or have none";
  }

  if (trueTransforms_.size() != 0 && falseTransforms_.size() != 0) {
    if (!util::contains({FeatureType::MatchCsv, FeatureType::MatchValue},
                        type_)) {
      return JPPS_INVALID_PARAMETER
             << name_ << ": only matching features can have branches";
    }
  }

  if (type_ == FeatureType::Codepoint) {
    if (intParam_ == 0) {
      return JPPS_INVALID_PARAMETER
             << name_ << ": feature can't point to the word itself";
    }
  }

  return Status::Ok();
}

Status UnkProcBuilder::validate() const {
  if (name_.size() == 0) {
    return JPPS_INVALID_PARAMETER << "unk processor must have a name";
  }
  if (type_ == UnkMakerType::Invalid) {
    return JPPS_INVALID_PARAMETER << name_
                                  << ": unk processor was not initialized";
  }
  if (pattern_ == -1) {
    return JPPS_INVALID_PARAMETER
           << name_ << ": unk processor must have pattern specified";
  }
  if (pattern_ <= 0) {
    return JPPS_INVALID_PARAMETER
           << name_ << ": unk pattern row numbers start from 1, found "
           << pattern_;
  }
  return Status::Ok();
}

}  // namespace dsl
}  // namespace spec
}  // namespace core
}  // namespace jumanpp
