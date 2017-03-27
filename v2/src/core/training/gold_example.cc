//
// Created by Arseny Tolmachev on 2017/03/27.
//

#include "gold_example.h"

//
// Created by Arseny Tolmachev on 2017/03/23.
//

#include "core/analysis/unk_nodes_creator.h"

namespace jumanpp {
namespace core {
namespace training {

bool TrainingExampleAdapter::matchUnkNodeData(const analysis::ExtraNode *pNode,
                                              const ExampleNode &node) {
  i32 surfaceHash = analysis::hashUnkString(node.surface);
  auto &ndata = node.data;
  util::ArraySlice<i32> latNodeData{pNode->content, (u32)entries.entrySize()};
  for (int i = 0; i < ndata.size(); ++i) {
    auto latticeIdx = spec->fields[i].index;
    auto latVal = latNodeData.at(latticeIdx);
    if (latVal < 0) {
      if (latVal != surfaceHash) {
        return false;
      }
    } else {
      if (latVal != ndata[i]) {
        return false;
      }
    }
  }
  return true;
}

bool TrainingExampleAdapter::matchDicNodeData(const ExampleNode &node) {
  auto &ndata = node.data;
  for (int i = 0; i < ndata.size(); ++i) {
    auto latticeIdx = spec->fields[i].index;
    JPP_DCHECK_IN(latticeIdx, 0, entryBuffer.size());
    if (ndata[i] != entryBuffer[latticeIdx]) {
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
  auto surfaceIdx = spec->fields[spec->surfaceIdx].index;
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
        if (matchDicNodeData(node)) {
          return i;
        }
      }
    }
  }
  return -1;
}

void TrainingExampleAdapter::makeUnkTrainingNode(const ExampleNode &node) {
  auto unk = xtra->makeZeroedUnk();
  auto nodeData = xtra->nodeContent(unk);
  auto contHash = analysis::hashUnkString(node.surface);
  for (int i = 0; i < node.data.size(); ++i) {
    auto nodeVal = node.data[i];
    auto idx = spec->fields[i].index;
    if (nodeVal >= 0) {
      nodeData.at(idx) = nodeVal;
    } else {
      nodeData.at(idx) = contHash;
    }
  }
  unk->header.unk.contentHash = contHash;
  unk->header.unk.surface = node.surface;
}

int TrainingExampleAdapter::findNodeInBoundary(
    analysis::LatticeBoundary *pBoundary, const ExampleNode &node,
    analysis::LatticeNodePtr nodePtr) {
  int start = std::min<int>(pBoundary->localNodeCount() - 1, nodePtr.position);

  auto &exdata = spec->fields;

  for (int i = start; i >= 0; --i) {
    auto data = pBoundary->starts()->entryData().row(i);
    for (int j = 0; j < exdata.size(); ++j) {
      auto idx = exdata[j].index;
      if (data[idx] != node.data[j]) {
        goto nextItem;
      }
    }
    return i;
  nextItem:;
  }

  return -1;
}

}  // training
}  // core
}  // jumanpp
