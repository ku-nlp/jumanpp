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
#include "core/impl/field_reader.h"
#include "core/runtime_info.h"

namespace jumanpp {
namespace core {
namespace dic {

struct DictionaryField {
  i32 index;
  StringPiece name;
  spec::ColumnType columnType;
  impl::IntStorageReader postions;
  impl::StringStorageReader strings;
  StringPiece emptyValue;
  i32 stringStorageIdx;
  bool isSurfaceField;
};

class FieldsHolder {
  std::vector<DictionaryField> fields_;

 public:
  const DictionaryField& at(i32 field) const {
    JPP_DCHECK_GE(field, 0);
    JPP_DCHECK_LT(field, fields_.size());
    return fields_[field];
  }

  const DictionaryField* byName(StringPiece name) const {
    for (auto& f : fields_) {
      if (f.name == name) {
        return &f;
      }
    }
    return nullptr;
  }

  Status load(const BuiltDictionary& dic);

  u32 totalFields() const { return fields_.size(); }
};

class DictionaryHolder {
  EntriesHolder entries_;
  FieldsHolder fields_;

 public:
  const DictionaryField* fieldByName(StringPiece name) const {
    return fields_.byName(name);
  }

  const FieldsHolder& fields() const { return fields_; }
  DictionaryEntries entries() const { return DictionaryEntries{&entries_}; }

  Status compileRuntimeInfo(const spec::AnalysisSpec& spec,
                            RuntimeInfo* runtime);

  Status load(const BuiltDictionary& dic);
};

Status fillEntriesHolder(const BuiltDictionary& dic, EntriesHolder* result);

}  // namespace dic
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_DICTIONARY_H
