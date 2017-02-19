//
// Created by Arseny Tolmachev on 2017/02/19.
//

#ifndef JUMANPP_DIC_SPEC_PARSER_H
#define JUMANPP_DIC_SPEC_PARSER_H

#include <util/string_piece.h>
#include <string>
#include <util/status.hpp>
#include <util/types.hpp>
#include <vector>

namespace jumanpp {
namespace core {
namespace dic {

enum class ColumnType { String, Int, StringList, Error };

struct ColumnDescriptor {
  i32 position;
  std::string name;
  bool isTrieKey;
  ColumnType columnType;
  std::string separatorRegex;
};

struct DictionarySpec {
  std::vector<ColumnDescriptor> columns;
  i32 indexColumn = -1;
};

Status parseDescriptor(StringPiece data, DictionarySpec* result);

}  // dic
}  // core
}  // jumanpp

#endif  // JUMANPP_DIC_SPEC_PARSER_H
