//
// Created by Arseny Tolmachev on 2017/05/31.
//

#include "rnn_id_resolver.h"
#include "util/serialization.h"
#include "util/serialization_flatmap.h"

namespace jumanpp {
namespace core {
namespace analysis {
namespace rnn {

Status RnnIdResolver::loadFromDic(const dic::DictionaryHolder& dic,
                                  StringPiece colName,
                                  util::ArraySlice<StringPiece> rnnDic,
                                  StringPiece unkToken) {
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
    if (s == unkToken) {
      unkId_ = i;
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
    for (int j = 0; j < entries.numRows(); ++j) {
      auto myEntry = entries.row(j).at(targetIdx_);
      auto rnnId = resolveId(myEntry, bnd, j, xtra);
      auto surfaceLength = starts->nodeInfo().at(j).numCodepoints();
      adder.add(rnnId, surfaceLength);
    }
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
    auto nodeInfo = lb->starts()->nodeInfo().at(position);
    auto node = xtra->node(nodeInfo.entryPtr());
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

void RnnIdResolver::serializeMaps(util::CodedBuffer* intBuffer,
                                  util::CodedBuffer* stringBuffer) const {
  util::serialization::Saver intSaver{intBuffer};
  intSaver.save(intMap_);
  util::serialization::Saver stringSaver{stringBuffer};
  stringSaver.save(strMap_);
}

Status RnnIdResolver::loadFromBuffers(StringPiece intBuffer,
                                      StringPiece stringBuffer, u32 targetIdx,
                                      u32 unkId) {
  util::serialization::Loader intLdr{intBuffer};
  if (!intLdr.load(&intMap_)) {
    return Status::InvalidState() << "RnnIdResolver: failed to load int map";
  }

  util::serialization::Loader stringLdr{stringBuffer};
  if (!stringLdr.load(&strMap_)) {
    return Status::InvalidState() << "RnnIdResolver: failed to load string map";
  }

  targetIdx_ = targetIdx;
  unkId_ = unkId;

  return Status::Ok();
}

}  // namespace rnn
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp