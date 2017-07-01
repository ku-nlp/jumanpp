//
// Created by Arseny Tolmachev on 2017/05/31.
//

#include "rnn_id_resolver.h"

namespace jumanpp {
namespace core {
namespace analysis {
namespace rnn {

Status RnnIdResolver::loadFromDic(const dic::DictionaryHolder& dic,
                                  StringPiece colName,
                                  util::ArraySlice<StringPiece> rnnDic) {
  auto fld = dic.fieldByName(colName);
  targetIdx_ = fld->index;
  if (fld->columnType != spec::ColumnType::String) {
    return Status::InvalidState()
           << "RNN import: field " << colName << " was not strings";
  }

  util::FlatMap<StringPiece, i32> str2int;

  dic::impl::StringStorageTraversal sst{fld->strings.data()};
  StringPiece data;
  while (sst.next(&data)) {
    str2int[data] = sst.position();
  }

  for (u32 i = 0; i < rnnDic.size(); ++i) {
    auto s = rnnDic[i];
    if (s.size() < 1) {
      continue;
    }
    auto it = str2int.find(s);
    if (it != str2int.end()) {
      intMap_[it->second] = i;
    } else {
      strMap_[s] = i;
    }
  }

  return Status::Ok();
}

Status RnnIdResolver::resolveIds(RnnIdContainer* ids, Lattice* lat,
                                 const ExtraNodesContext* xtra) const {
  ids->reset();
  ids->alloc(lat->createdBoundaryCount());

  RnnIdAdder bos0_adder{ids, 0};
  bos0_adder.add(0, 0);
  bos0_adder.finish();
  RnnIdAdder bos1_adder{ids, 1};
  bos1_adder.add(0, 0);
  bos1_adder.finish();

  int bcnt = lat->createdBoundaryCount();
  for (int i = 2; i < bcnt - 1; ++i) {
    auto bnd = lat->boundary(i);
    auto starts = bnd->starts();
    auto entries = starts->entryData();
    RnnIdAdder adder{ids, i};
    // if (bnd->isActive()) {
    for (int j = 0; j < entries.numRows(); ++j) {
      auto myEntry = entries.row(j).at(targetIdx_);
      auto rnnId = resolveId(myEntry, bnd, j, xtra);
      adder.add(rnnId, 1);  // TODO: length
    }
    //}
    adder.finish();
  }

  RnnIdAdder eos_adder{ids, bcnt - 1};
  eos_adder.add(0, 0);
  eos_adder.finish();
  RnnIdAdder guard{ids, bcnt};

  return Status::Ok();
}

i32 RnnIdResolver::resolveId(i32 entry, LatticeBoundary* lb, int position,
                             const ExtraNodesContext* xtra) const {
  if (entry < 0) {
    // We have an OOV
    // But it can be present in RNN model
    auto eptr = lb->starts()->entryPtrData().at(position);
    auto node = xtra->node(eptr);
    if (node->header.type == ExtraNodeType::Unknown) {
      StringPiece sp = node->header.unk.surface;
      if (sp.size() != 0) {
        auto it = strMap_.find(sp);
        if (it != strMap_.end()) {
          return it->second;
        }
      }
    }
  } else {
    // We have a regular word.
    // Check in id mapping
    auto i1 = intMap_.find(entry);
    if (i1 != intMap_.end()) {
      return i1->second;
    }

    // Otherwise it is a dictionary word which
    // is not present in RNN model.
  }
  return -1;
}

}  // namespace rnn
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp