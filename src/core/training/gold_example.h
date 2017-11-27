//
// Created by Arseny Tolmachev on 2017/03/27.
//

#ifndef JUMANPP_GOLD_EXAMPLE_H
#define JUMANPP_GOLD_EXAMPLE_H

#include "core/analysis/analysis_result.h"
#include "core/analysis/analyzer_impl.h"
#include "full_example.h"

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

  void reset() { goldPath.clear(); }
};

struct AllowedFieldInfo {
  util::FlatMap<i32, i32> unkPatternToGoldIdx_;
  i32 goldIdx_;
};

class UnkAllowedField {
  std::vector<AllowedFieldInfo> fields_;

 public:
  UnkAllowedField(const CoreHolder &core);
  bool isAllowed(const analysis::ExtraNode *xtra, const ExampleNode &ex) const;
};

class TrainingExampleAdapter {
  const spec::TrainingSpec *spec;
  analysis::Lattice *lattice;
  analysis::LatticeBuilder *latticeBuilder;
  analysis::ExtraNodesContext *xtra;
  dic::DictionaryEntries entries;
  std::vector<i32> entryBuffer;
  UnkAllowedField unkAllowed;

 public:
  TrainingExampleAdapter(analysis::AnalyzerImpl *impl);

  void reset() { entryBuffer.clear(); }

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
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp
#endif  // JUMANPP_GOLD_EXAMPLE_H
