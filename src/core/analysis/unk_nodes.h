//
// Created by Arseny Tolmachev on 2017/03/05.
//

#ifndef JUMANPP_UNK_NODES_H
#define JUMANPP_UNK_NODES_H

#include <memory>
#include "core/analysis/unk_maker_types.h"

namespace jumanpp {
namespace core {
class CoreHolder;

namespace analysis {

class AnalysisInput;
class UnkNodesContext;
class LatticeBuilder;

class UnkMaker {
 public:
  virtual bool spawnNodes(const AnalysisInput& input, UnkNodesContext* ctx,
                          LatticeBuilder* lattice) const = 0;
  virtual ~UnkMaker() = default;
};

struct UnkMakers {
  std::vector<std::unique_ptr<UnkMaker>> stage1;
  std::vector<std::unique_ptr<UnkMaker>> stage2;
};

Status makeMakers(const CoreHolder& core, UnkMakers* result);

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_UNK_NODES_H
