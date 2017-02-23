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
  }
  return Status::Ok();
}

Status ModelSpecBuilder::validate() const {
  JPP_RETURN_IF_ERROR(validateFields());
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
  if (columnType_ == dic::ColumnType::Error) {
    return Status::InvalidParameter() << "coumn " << name_
                                      << " do not have type";
  }
  if (trieIndex_ && columnType_ != dic::ColumnType::String) {
    return Status::InvalidParameter()
           << "only string field can be indexed, field " << this->name_
           << " is not one";
  }
  if (emptyValue_.size() > 0 && columnType_ == dic::ColumnType::Int) {
    return Status::InvalidParameter()
           << "empty field does not make sense on int fields like "
           << this->name_;
  }

  return Status::Ok();
}
}  // dsl
}  // spec
}  // core
}  // jumanpp
