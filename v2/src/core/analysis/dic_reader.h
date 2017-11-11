//
// Created by Arseny Tolmachev on 2017/03/01.
//

#ifndef JUMANPP_DIC_READER_H
#define JUMANPP_DIC_READER_H

#include "core/dic/dictionary.h"
#include "util/array_slice.h"
#include "util/memory.hpp"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

struct ExtraNode;

class DictNode {
  dic::DicEntryBuffer entry_;

  friend class DicReader;

 public:
  util::MutableArraySlice<i32> copyTo(ExtraNode* node) const;
  util::ArraySlice<i32> data() const { return entry_.features(); }
};

class DicReader {
  const dic::DictionaryHolder& dic_;

 public:
  DicReader(const dic::DictionaryHolder& holder) : dic_{holder} {}
  DictNode readEntry(EntryPtr ptr) const;
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_DIC_READER_H
