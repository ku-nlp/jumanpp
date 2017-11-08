//
// Created by Arseny Tolmachev on 2017/02/19.
//

#include "dictionary.h"
#include "util/flatmap.h"
#include "util/flatset.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace dic {

Status fillEntriesHolder(const BuiltDictionary& dic, EntriesHolder* result) {
  result->numFeatures = static_cast<i32>(dic.spec.features.numDicFeatures);
  result->numData = static_cast<i32>(dic.spec.features.numDicData);
  result->entries = impl::IntStorageReader{dic.entryData};
  result->entryPtrs = impl::IntStorageReader{dic.entryPointers};
  return result->trie.loadFromMemory(dic.trieContent);
}

Status FieldsHolder::load(const BuiltDictionary& dic) {
  for (i32 index = 0; index < dic.fieldData.size(); ++index) {
    auto& f = dic.fieldData[index];
    auto& sf = dic.spec.dictionary.fields[f.specIndex];
    DictionaryField df{index,
                       sf.dicIndex,
                       sf.name,
                       sf.fieldType,
                       impl::IntStorageReader{f.fieldContent},
                       impl::StringStorageReader{f.stringContent},
                       sf.emptyString,
                       f.stringStorageIdx,
                       sf.isTrieKey};
    fields_.push_back(df);
  }
  return Status::Ok();
}

Status DictionaryHolder::load(const BuiltDictionary& dic) {
  JPP_RETURN_IF_ERROR(fields_.load(dic));
  JPP_RETURN_IF_ERROR(fillEntriesHolder(dic, &entries_));
  return Status::Ok();
}

}  // namespace dic
}  // namespace core
}  // namespace jumanpp