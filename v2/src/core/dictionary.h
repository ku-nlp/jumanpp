//
// Created by Arseny Tolmachev on 2017/02/19.
//

#ifndef JUMANPP_DICTIONARY_H
#define JUMANPP_DICTIONARY_H

#include <memory>
#include <vector>
#include "dic_spec.h"

namespace jumanpp {
namespace core {
namespace dic {

using DicItemId = i32;

class EntryDescriptor;

class DictionaryBuilder {
  DictionarySpec* spec_;
  std::shared_ptr<EntryDescriptor> descriptor_;

 public:
  Status importFromMemory(StringPiece data, StringPiece name);
};

class EntryDescriptor {
  /**
   * Contains offsets for each field
   */
  std::vector<i32> order_;
  DictionarySpec const* spec_;

 public:
  Status build(const DictionarySpec& spec);
};

class DictionaryEntry {
  DicItemId* items_;
  EntryDescriptor descr_;
};

}  // dic
}  // core
}  // jumanpp

#endif  // JUMANPP_DICTIONARY_H
