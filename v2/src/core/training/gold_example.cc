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
  util::ArraySlice<i32> latNodeData{pNode->content, (u32)entries.entrySize()};
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
    auto latticeIdx = trainFld.fieldIdx;
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
  return unk->ptr();
}

int TrainingExampleAdapter::findNodeInBoundary(
    analysis::LatticeBoundary *pBoundary, const ExampleNode &node,
    analysis::LatticeNodePtr nodePtr) {
  int start = std::min<int>(pBoundary->localNodeCount() - 1, nodePtr.position);

  auto contentHash = analysis::hashUnkString(node.surface);
  auto &exdata = spec->fields;

  for (int i = start; i >= 0; --i) {
    auto data = pBoundary->starts()->entryData().row(i);
    for (int j = 0; j < exdata.size(); ++j) {
      auto &fldInfo = exdata[j];
      if (fldInfo.weight == 0) {
        continue;
      }
      auto idx = fldInfo.fieldIdx;
      auto latticeItem = data[idx];
      i32 refItem = 0;
      if (latticeItem < 0) {
        refItem = contentHash;
      } else {
        refItem = node.data.at(fldInfo.number);
      }
      if (latticeItem != refItem) {
        goto nextItem;
      }
    }
    return i;
  nextItem:;
  }

  return -1;
}

Status TrainingExampleAdapter::repointPathPointers(
    const FullyAnnotatedExample &ex, GoldenPath *path) {
  for (int i = 0; i < ex.numNodes(); ++i) {
    auto exNode = ex.nodeAt(i);
    auto ptr = path->nodes().at(i);
    auto bnd = lattice->boundary(ptr.boundary);
    JPP_DCHECK_EQ(bnd->localNodeCount(),
                  latticeBuilder->infoAt(ptr.boundary - 2).startCount);
    int idxOf = findNodeInBoundary(bnd, exNode, ptr);
    if (idxOf == -1) {
      LOG_TRACE() << "NODE=" << VOut(exNode.data);

      auto sts = bnd->starts();
      for (int j = 0; j < sts->arraySize(); ++j) {
        auto entr = sts->entryData().row(j);
        LOG_TRACE() << "LATTICE[" << j << "]=" << VOut(entr);
      }

      return Status::InvalidState()
             << "could not find gold node in boundary #" << ptr.boundary;
    }
    path->fixBoundaryPos(i, idxOf);
  }
  return Status::Ok();
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp
