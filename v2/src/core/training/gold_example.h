//
// Created by Arseny Tolmachev on 2017/03/27.
//

#ifndef JUMANPP_GOLD_EXAMPLE_H
#define JUMANPP_GOLD_EXAMPLE_H

#include "core/analysis/analysis_result.h"
#include "core/analysis/analyzer_impl.h"
#include "training_io.h"

namespace jumanpp {
namespace core {
namespace training {

class GoldenPath {
  std::vector<analysis::LatticeNodePtr> goldPath;

 public:
  void addGoldNode(i32 boundary, i32 position) {
    // + 2, we have 2 BOS nodes
    u16 intBnd = static_cast<u16>(boundary + 2);
    u16 intPos = static_cast<u16>(position);
    goldPath.push_back({intBnd, intPos});
  }

  util::ArraySlice<analysis::LatticeNodePtr> nodes() const { return goldPath; }

  void fixBoundaryPos(int nodeIdx, int newPosition) {
    goldPath[nodeIdx].position = static_cast<u16>(newPosition);
  }
};

class TrainingExampleAdapter {
  const spec::TrainingSpec *spec;
  analysis::Lattice *lattice;
  analysis::LatticeBuilder *latticeBuilder;
  analysis::ExtraNodesContext *xtra;
  dic::DictionaryEntries entries;
  std::vector<i32> entryBuffer;

 public:
  TrainingExampleAdapter(const spec::TrainingSpec *spec,
                         analysis::AnalyzerImpl *impl)
      : spec{spec},
        lattice{impl->lattice()},
        latticeBuilder{impl->latticeBldr()},
        xtra{impl->extraNodesContext()},
        entries{impl->dic().entries()} {}

  bool matchDicNodeData(const ExampleNode &node);
  void loadDicSeedData(const analysis::LatticeNodeSeed &seed) {
    entryBuffer.clear();
    entries.entryAtPtr(seed.entryPtr).fill(entryBuffer);
  }

  bool matchUnkNodeData(const analysis::ExtraNode *pNode,
                        const ExampleNode &node);
  int nodeSeedExists(const ExampleNode &node);
  EntryPtr makeUnkTrainingNode(const ExampleNode &node);

  /**
   * Resolve or add new nodes for training.
   * Should be called after LatticeBuilder has all the NodeSeeds present,
   * but before NodeSeed with the same representation get compressed to a
   * single node.
   *
   * This function will add new seeds to LatticeBuilder for nodes in training
   * example which were not present in the LatticeBuilder.
   *
   * Probably, training will heavily overfit those nodes, I wonder what can be
   * done in that regard.
   *
   * @return whether at least one node have been created
   */
  bool ensureNodes(const FullyAnnotatedExample &ex, GoldenPath *path) {
    bool createdNewNode = false;
    for (int i = 0; i < ex.numNodes(); ++i) {
      auto exNode = ex.nodeAt(i);
      int nodePos = nodeSeedExists(exNode);
      int boundary = exNode.position;
      if (nodePos < 0) {
        auto &bnfo = latticeBuilder->infoAt(boundary);
        auto size = bnfo.startCount;
        auto eptr = makeUnkTrainingNode(exNode);
        path->addGoldNode(boundary, size);
        u16 endPos = static_cast<u16>(bnfo.number + exNode.length);
        latticeBuilder->appendSeed(eptr, bnfo.number, endPos);
        createdNewNode = true;
      } else {
        path->addGoldNode(boundary, nodePos);
      }
    }
    return createdNewNode;
  }

  int findNodeInBoundary(analysis::LatticeBoundary *pBoundary,
                         const ExampleNode &node,
                         analysis::LatticeNodePtr nodePtr);

  /**
     * After the Lattice is built, we need to fixup pointers of GoldenPath.
     * Equivalent node compression could have invalidated our pointers, so
     * recheck them.
     */
  Status repointPathPointers(const FullyAnnotatedExample &ex,
                             GoldenPath *path) {
    for (int i = 0; i < ex.numNodes(); ++i) {
      auto exNode = ex.nodeAt(i);
      auto ptr = path->nodes().at(i);
      auto bnd = lattice->boundary(ptr.boundary);
      int idxOf = findNodeInBoundary(bnd, exNode, ptr);
      if (idxOf == -1) {
        return Status::InvalidState()
               << "could not find gold node in boundary #" << ptr.boundary;
      }
      path->fixBoundaryPos(i, idxOf);
    }
    return Status::Ok();
  }
};

}  // training
}  // core
}  // jumanpp
#endif  // JUMANPP_GOLD_EXAMPLE_H
