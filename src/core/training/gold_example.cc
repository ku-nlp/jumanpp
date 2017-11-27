//
// Created by Arseny Tolmachev on 2017/03/27.
//

#include "gold_example.h"

//
// Created by Arseny Tolmachev on 2017/03/23.
//

#include "core/analysis/unk_nodes_creator.h"
#include "util/debug_output.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace training {

bool TrainingExampleAdapter::matchUnkNodeData(const analysis::ExtraNode *pNode,
                                              const ExampleNode &node) {
  i32 surfaceHash = analysis::hashUnkString(node.surface);
  auto &ndata = node.data;
  util::ArraySlice<i32> latNodeData{pNode->content, (u32)entries.numFeatures()};
  for (int i = 0; i < ndata.size(); ++i) {
    auto &trainFld = spec->fields[i];
    if (trainFld.weight == 0) {
      continue;
    }
    auto latticeIdx = trainFld.fieldIdx;
    auto latVal = latNodeData.at(latticeIdx);
    if (latVal < 0) {
      if (latVal != surfaceHash) {
        return false;
      }
    } else {
      if (latVal != ndata.at(trainFld.number)) {
        return false;
      }
    }
  }
  return true;
}

bool TrainingExampleAdapter::matchDicNodeData(const ExampleNode &node) {
  auto &ndata = node.data;

  for (int i = 0; i < ndata.size(); ++i) {
    auto &trainFld = spec->fields[i];
    if (trainFld.weight == 0) {
      continue;
    }
    auto latticeIdx = trainFld.dicIdx;
    JPP_DCHECK_IN(latticeIdx, 0, entryBuffer.size());
    if (ndata.at(trainFld.number) != entryBuffer[latticeIdx]) {
      return false;
    }
  }
  return true;
}

int TrainingExampleAdapter::nodeSeedExists(const ExampleNode &node) {
  auto seeds = latticeBuilder->seeds();
  auto nfo = latticeBuilder->infoAt(node.position);
  util::ArraySlice<analysis::LatticeNodeSeed> subset{seeds, nfo.firstNodeOffset,
                                                     (u32)nfo.startCount};
  auto surfaceIdx = spec->fields[spec->surfaceIdx].number;
  EntryPtr surface{node.data.at(surfaceIdx)};

  if (surface.isSpecial()) {
    auto content = node.surface;

    // pass 1: try to find a regular unk
    for (int i = 0; i < subset.size(); ++i) {
      auto &seed = subset[i];
      if (seed.entryPtr.isSpecial()) {
        auto latNode = xtra->node(seed.entryPtr);
        if (latNode->header.unk.surface == content) {
          if (matchUnkNodeData(latNode, node)) {
            return i;
          }
        }
      }
    }

    // pass 2: try to find an acceptable unk (the first one)
    for (int i = 0; i < subset.size(); ++i) {
      auto &seed = subset[i];
      if (seed.entryPtr.isSpecial()) {
        auto latNode = xtra->node(seed.entryPtr);
        if (latNode->header.unk.surface == content) {
          if (unkAllowed.isAllowed(latNode, node)) {
            return i;
          }
        }
      }
    }
  } else {
    for (int i = 0; i < subset.size(); ++i) {
      auto &seed = subset[i];
      if (seed.entryPtr.isDic()) {
        loadDicSeedData(seed);
        // LOG_TRACE() << "comparing gold=" << VOut(node.data) << " entry=" <<
        // VOut(entryBuffer) << " at " << node.position;
        if (matchDicNodeData(node)) {
          return i;
        }
      }
    }
  }
  return -1;
}

EntryPtr TrainingExampleAdapter::makeUnkTrainingNode(const ExampleNode &node) {
  auto unk = xtra->makeZeroedUnk();
  auto nodeData = xtra->nodeContent(unk);
  auto contHash = analysis::hashUnkString(node.surface);
  for (int i = 0; i < node.data.size(); ++i) {
    auto nodeVal = node.data[i];
    auto idx = spec->fields[i].fieldIdx;
    if (nodeVal >= 0) {
      nodeData.at(idx) = nodeVal;
    } else {
      nodeData.at(idx) = contHash;
    }
  }
  unk->header.unk.contentHash = contHash;
  unk->header.unk.surface = node.surface;
  unk->header.unk.templatePtr = EntryPtr{0};
  return unk->ptr();
}

TrainingExampleAdapter::TrainingExampleAdapter(analysis::AnalyzerImpl *impl)
    : spec{&impl->core().spec().training},
      lattice{impl->lattice()},
      latticeBuilder{impl->latticeBldr()},
      xtra{impl->extraNodesContext()},
      entries{impl->dic().entries()},
      unkAllowed{impl->core()} {}

bool UnkAllowedField::isAllowed(const analysis::ExtraNode *xtra,
                                const ExampleNode &ex) const {
  for (auto &f : fields_) {
    auto v = ex.data.at(f.goldIdx_);
    auto unkPat = xtra->header.unk.templatePtr.rawValue();
    auto it = f.unkPatternToGoldIdx_.find(unkPat);
    if (it == f.unkPatternToGoldIdx_.end()) {
      return false;
    }
    if (it->second != v) {
      return false;
    }
  }
  return true;
}

UnkAllowedField::UnkAllowedField(const CoreHolder &core) {
  auto &tspec = core.spec().training;
  auto &flds = core.spec().dictionary.fields;
  auto &unks = core.spec().unkCreators;
  auto &spec = tspec.allowedUnk;
  for (auto &f : spec) {
    auto &srcF = flds[f.sourceField];
    auto &trgF = flds[f.targetField];
    auto fld = core.dic().fieldByName(srcF.name);
    auto trgFldData = core.dic().fieldByName(trgF.name);

    if (srcF.dicIndex >= 0) {
      LOG_WARN() << "source field was in feature section";
      continue;
    }

    this->fields_.emplace_back();
    auto &afi = fields_.back();

    for (auto &u : unks) {
      auto ptr = u.patternPtr;
      dic::DicEntryBuffer deb;
      if (!core.dic().entries().fillBuffer(EntryPtr{ptr}, &deb)) {
        LOG_WARN() << "failed to read an entry from dic";
        continue;
      }
      if (!deb.nextData()) {
        LOG_WARN() << "failed to read an entry data";
        continue;
      }
      auto fldPtr = deb.data().at(~srcF.dicIndex);
      auto kv = fld->postions.kvListAt(fldPtr);
      while (kv.moveNext()) {
        auto kid = kv.key();
        StringPiece sp;
        if (!fld->strings.readAt(kid, &sp)) {
          LOG_WARN() << "failed to read key value";
          continue;
        }
        if (sp != f.sourceKey) {
          continue;
        }

        if (!fld->strings.readAt(kv.value(), &sp)) {
          LOG_WARN() << "feiled to read value from dic";
          continue;
        }

        auto it2 = dic::impl::StringStorageTraversal{trgFldData->strings};
        StringPiece sp2;
        while (it2.next(&sp2)) {
          if (sp2 == sp) {
            afi.unkPatternToGoldIdx_[ptr] = it2.position();
          }
        }
      }
    }

    for (auto &tf : tspec.fields) {
      if (tf.fieldIdx == trgF.specIndex) {
        afi.goldIdx_ = tf.number;
      }
    }
  }
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp
