//
// Created by Hajime Morita on 2017/07/17.
//

#ifndef JUMANPP_NORMALIZED_NODE_CREATOR_H
#define JUMANPP_NORMALIZED_NODE_CREATOR_H

#include "core/analysis/analysis_input.h"
#include "core/analysis/charlattice.h"
#include "core/analysis/dic_reader.h"
#include "core/analysis/extra_nodes.h"
#include "core/analysis/lattice_builder.h"
#include "core/analysis/unk_nodes.h"
#include "unk_nodes_creator.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

class NormalizedNodeMaker : public UnkMaker {
  dic::DictionaryEntries entries_;
  UnkNodeConfig info_;

  using CodepointStorage = std::vector<jumanpp::chars::InputCodepoint>;
  using Codepoint = jumanpp::chars::InputCodepoint;

  static const LatticePosition MaxLength = 64;

 public:
  NormalizedNodeMaker(const dic::DictionaryEntries& entries_,
                      UnkNodeConfig&& info_);
  bool spawnNodes(const AnalysisInput& input, UnkNodesContext* ctx,
                  LatticeBuilder* lattice) const override;
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_NORMALIZED_NODE_CREATOR_H
