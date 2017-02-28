//
// Created by Arseny Tolmachev on 2017/02/19.
//

#ifndef JUMANPP_DICTIONARY_H
#define JUMANPP_DICTIONARY_H

#include <memory>
#include <vector>
#include "core/core_types.h"
#include "core/dic_builder.h"
#include "core/dic_entries.h"
#include "core/spec/spec_types.h"

namespace jumanpp {
namespace core {
namespace dic {

struct DictionaryField {
  i32 index;
  i32 csvPosition;
  StringPiece name;
  spec::ColumnType columnType;
  impl::IntStorageReader postions;
  impl::StringStorageReader strings;
  bool usesPositions;
};

class FieldsHolder {
  std::vector<DictionaryField> fields_;

 public:
  const DictionaryField &at(i32 field) const {
    JPP_DCHECK_GE(field, 0);
    JPP_DCHECK_LT(field, fields_.size());
    return fields_[field];
  }
};

class DictionaryHolder {
  spec::AnalysisSpec spec_;
  EntriesHolder entries_;
  FieldsHolder fields_;
};

Status fillEntriesHolder(const BuiltDictionary &dic, EntriesHolder *result);

}  // dic
}  // core
}  // jumanpp

#endif  // JUMANPP_DICTIONARY_H
