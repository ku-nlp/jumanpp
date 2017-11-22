//
// Created by Arseny Tolmachev on 2017/11/07.
//

#ifndef JUMANPP_FEATURE_DEBUG_H
#define JUMANPP_FEATURE_DEBUG_H

#include <string>
#include <vector>
#include "core/analysis/output.h"
#include "core/analysis/score_api.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {
class Lattice;
class AnalyzerImpl;
}  // namespace analysis
namespace features {

struct DebugFeatureInstance {
  u32 ngramIdx;
  std::string repr;
  u64 rawHashCode;
  u32 maskedHashCode;
  float score;
};

struct DebugFeatures {
  std::vector<DebugFeatureInstance> features;
};

struct StringAppender {
  virtual ~StringAppender() = default;
  virtual bool appendTo(std::string* resut,
                        const analysis::NodeWalker& walker) = 0;
};

struct NodeTriple {
  analysis::LatticeNodePtr t2;
  analysis::LatticeNodePtr t1;
  analysis::LatticeNodePtr t0;
};

class FeaturesDebugger {
  const analysis::WeightBuffer* weights_;
  const analysis::Lattice* lattice_;
  std::vector<std::unique_ptr<StringAppender>> appenders_;
  analysis::OutputManager output_;

 public:
  Status initialize(const analysis::AnalyzerImpl* impl,
                    const analysis::WeightBuffer& buf);
  Status fill(DebugFeatures* result, const NodeTriple& nodes);
  Status fill(DebugFeatures* result,
              const analysis::ConnectionBeamElement& beam);
  Status fill(DebugFeatures* result, const analysis::ConnectionPtr& ptr);
};

}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_DEBUG_H
