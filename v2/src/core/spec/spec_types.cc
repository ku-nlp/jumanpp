//
// Created by Arseny Tolmachev on 2017/02/28.
//

#include <iostream>
#include "spec_types.h"

std::ostream &::jumanpp::core::spec::operator<<(std::ostream &o, jumanpp::core::spec::ColumnType ct) {
  switch (ct) {
    case ColumnType::String:
      o << "String";
      break;
    case ColumnType::Int:
      o << "Int";
      break;
    case ColumnType::StringList:
      o << "StringList";
      break;
    case ColumnType::Error:
      o << "Error";
      break;
  }
  return o;
}
