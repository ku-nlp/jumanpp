//
// Created by Arseny Tolmachev on 2017/02/19.
//

#ifndef JUMANPP_DICTIONARY_H
#define JUMANPP_DICTIONARY_H

#include <memory>
#include <vector>
#include "dic_entries.h"
#include "dic_spec.h"

namespace jumanpp {
namespace core {
namespace dic {

struct DictionaryField {
  i32 index;
  i32 csvPosition;
  StringPiece name;
  ColumnType columnType;
  impl::IntStorageReader postions;
  impl::StringStorageReader strings;
  bool usesPositions;
};

class FieldsHolder {
  std::vector<DictionaryField> fields_;
};

class DictionaryHolder {
  DictionarySpec spec_;
  EntriesHolder entries_;
  FieldsHolder fields_;
};

}  // dic
}  // core
}  // jumanpp

#endif  // JUMANPP_DICTIONARY_H
