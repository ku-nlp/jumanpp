//
// Created by Arseny Tolmachev on 2017/02/23.
//

#ifndef JUMANPP_SPEC_TYPES_H
#define JUMANPP_SPEC_TYPES_H

#include <vector>
#include "util/string_piece.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace spec {

enum class ColumnType { String, Int, StringList, Error };

std::ostream& operator<<(std::ostream& o, ColumnType ct);

struct FieldDescriptor {
  i32 index = -1;
  i32 position;
  std::string name;
  bool isTrieKey = false;
  ColumnType columnType = ColumnType::Error;
  StringPiece emptyString = "";
};

struct AnalysisSpec {
  std::vector<FieldDescriptor> columns;
  i32 indexColumn = -1;
};

}  // spec
}  // core
}  // jumanpp

#endif  // JUMANPP_SPEC_TYPES_H
