//
// Created by Arseny Tolmachev on 2017/02/19.
//

#include "dictionary.h"

namespace jumanpp {
namespace core {
namespace dic {

Status fillEntriesHolder(const BuiltDictionary &dic, EntriesHolder *result) {
  result->entrySize = static_cast<i32>(dic.fieldData.size());
  result->entries = impl::IntStorageReader{dic.entryData};
  result->entryPtrs = impl::IntStorageReader{dic.entryPointers};
  return result->trie.loadFromMemory(dic.trieContent);
}

Status FieldsHolder::load(const BuiltDictionary &dic) {
  for (i32 index = 0; index < dic.fieldData.size(); ++index) {
    auto &f = dic.fieldData[index];
    DictionaryField df{index,
                       f.name,
                       f.colType,
                       impl::IntStorageReader{f.fieldContent},
                       impl::StringStorageReader{f.stringContent},
                       f.usesFieldContent};
    fields_.push_back(df);
  }
  return Status::Ok();
}

Status DictionaryHolder::load(const BuiltDictionary &dic) {
  JPP_RETURN_IF_ERROR(fields_.load(dic));
  JPP_RETURN_IF_ERROR(fillEntriesHolder(dic, &entries_));
  return Status::Ok();
}
}  // dic
}  // core
}  // jumanpp