//
// Created by Arseny Tolmachev on 2017/02/28.
//

#include "spec_types.h"
#include <iostream>

std::ostream & ::jumanpp::core::spec::operator<<(
    std::ostream &o, jumanpp::core::spec::FieldType ct) {
  switch (ct) {
    case FieldType::String:
      o << "String";
      break;
    case FieldType::Int:
      o << "Int";
      break;
    case FieldType::StringList:
      o << "StringList";
      break;
    case FieldType::StringKVList:
      o << "StringKVList";
      break;
    case FieldType::Error:
      o << "Error";
      break;
  }
  return o;
}

jumanpp::Status jumanpp::core::spec::AnalysisSpec::validate() const {
  if (specMagic_ != SpecMagic || specMagic2_ != SpecMagic) {
    return JPPS_INVALID_STATE
           << "Spec was corrupted, magic number was not correct";
  }
  if (specVersion_ != SpecFormatVersion) {
    return JPPS_INVALID_STATE
           << "Spec version is incompatible, was " << specVersion_
           << " but this binary can work only with " << SpecFormatVersion;
  }
  return jumanpp::Status::Ok();
}
