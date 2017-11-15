//
// Created by Arseny Tolmachev on 2017/11/15.
//

#include <iostream>
#include "core/core_types.h"
#include "core/dic/dic_builder.h"
#include "core/dic/dic_entries.h"
#include "core/dic/dictionary.h"
#include "core/dic/field_reader.h"
#include "core/impl/model_io.h"
#include "util/flatmap.h"
#include "util/memory.hpp"

using namespace jumanpp;

int varintSize(u32 v) {
  if (v < (1 << 7)) {
    return 1;
  } else if (v < (1 << 14)) {
    return 2;
  } else if (v < (1 << 21)) {
    return 3;
  } else if (v < (1 << 28)) {
    return 4;
  } else {
    return 5;
  }
}

struct SizeStatus {
  size_t storage;
  size_t index;
};

struct AlignmentStatus {
  SizeStatus forAligns[8];
  util::FlatMap<i32, i32> counts;

  AlignmentStatus() { std::memset(forAligns, 0, sizeof(forAligns)); }

  void addStorage(i32 v, i32 size) {
    for (int align = 0; align < 8; ++align) {
      auto newSize = util::memory::Align(size + varintSize(size), 1u << align);
      auto ptr = forAligns[align].storage;
      forAligns[align].storage += newSize;
      forAligns[align].index += varintSize(ptr >> align) * counts[v];
    }
  }

  void addCount(i32 v) { counts[v] += 1; }
};

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "must pass 2 parameters\n";
    return 1;
  }

  auto filename = StringPiece::fromCString(argv[1]);
  auto fieldName = StringPiece::fromCString(argv[2]);

  core::model::FilesystemModel fsm;
  auto s = fsm.open(filename);
  if (!s) {
    std::cerr << "failed to open model: " << s;
    return 1;
  }

  core::model::ModelInfo info;
  if (!fsm.load(&info)) {
    return 1;
  }

  core::dic::BuiltDictionary dic;
  if (!dic.restoreDictionary(info)) {
    return 1;
  }

  StringPiece raw{};

  std::vector<i32> fields;
  bool hasDicData = false;
  i32 storageIdx;

  for (auto& f : dic.spec.dictionary.fields) {
    if (f.name == fieldName) {
      if (f.stringStorage == -1) {
        return 2;
      }
      raw = dic.stringStorages[f.stringStorage];
      storageIdx = f.stringStorage;

      if (f.dicIndex < 0) {
        hasDicData = true;
      }
      break;
    }
  }

  for (auto& f : dic.spec.dictionary.fields) {
    if (f.stringStorage == storageIdx) {
      fields.push_back(f.dicIndex);
    }
  }

  core::dic::DictionaryHolder holder;
  holder.load(dic);

  AlignmentStatus alignStatus;

  core::dic::impl::IntStorageReader ptrReader{dic.entryPointers};
  i32 idxPtr = 0;
  while (idxPtr < dic.entryPointers.size()) {
    auto ptrs = ptrReader.listAt(idxPtr);
    i32 eptr = 0;
    core::dic::DicEntryBuffer buffer;
    while (!ptrs.empty() && ptrs.readOneCumulative(&eptr)) {
      core::EntryPtr entryPtr{eptr};
      holder.entries().fillBuffer(entryPtr, &buffer);
      for (auto f : fields) {
        if (f >= 0) {
          alignStatus.addCount(buffer.features().at(f));
        }
      }
      while (hasDicData && buffer.nextData()) {
        for (auto f : fields) {
          if (f < 0) {
            alignStatus.addCount(buffer.data().at(~f));
          }
        }
      }
    }
    idxPtr += ptrs.numReadBytes();
  }

  core::dic::impl::StringStorageTraversal trav{raw};
  StringPiece res;
  while (trav.next(&res)) {
    alignStatus.addStorage(trav.position(), res.size());
  }

  int align = 0;
  auto total1 =
      alignStatus.forAligns[0].index + alignStatus.forAligns[0].storage;
  for (auto& as : alignStatus.forAligns) {
    auto total = as.index + as.storage;
    float perc = (total / static_cast<float>(total1)) * 100;

    std::cout << "for align " << (1 << align) << " storage: " << as.storage
              << " index " << as.index << " total: " << total
              << " perc: " << perc << "%\n";
    align += 1;
  }
}