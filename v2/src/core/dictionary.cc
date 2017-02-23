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

}  // dic
}  // core
}  // jumanpp