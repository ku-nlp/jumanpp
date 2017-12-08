//
// Created by Arseny Tolmachev on 2017/11/07.
//

#ifndef JUMANPP_FEATURE_DEBUG_H
#define JUMANPP_FEATURE_DEBUG_H

#include <string>
#include <vector>
#include "core/analysis/output.h"
#include "core/analysis/score_api.h"
#include "core/impl/feature_computer.h"
#include "core/impl/feature_impl_prim.h"
#include "util/fast_printer.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {
class Lattice;
class AnalyzerImpl;
}  // namespace analysis
namespace features {

struct DebugFeatureInstance {
  i32 ngramIdx;
  std::string repr;
  u32 rawHashCode;
  u32 maskedHashCode;
  float score;
};

struct DebugFeatures {
  std::vector<DebugFeatureInstance> features;
};

struct Display {
  virtual ~Display() = default;
  virtual bool appendTo(util::io::FastPrinter& p,
                        const analysis::NodeWalker& walker, const NodeInfo& ni,
                        impl::PrimitiveFeatureContext* ctx) = 0;
};

class FeaturesDebugger {
  const analysis::WeightBuffer* weights_;
  std::vector<std::unique_ptr<Display>> combines_;
  std::vector<std::unique_ptr<Display>> patterns_;
  std::vector<u32> featureBuf_;
  util::io::FastPrinter printer_;

  Status makeCombine(const analysis::AnalyzerImpl* impl, i32 idx,
                     std::unique_ptr<Display>& result);
  Status makePrimitive(const analysis::AnalyzerImpl* impl, i32 idx,
                       std::unique_ptr<Display>& result);

 public:
  Status initialize(const analysis::AnalyzerImpl* impl,
                    const analysis::WeightBuffer& buf);

  Status fill(const analysis::AnalyzerImpl* impl, DebugFeatures* result,
              const NgramFeatureRef& nodes);
  Status fill(const analysis::AnalyzerImpl* impl, DebugFeatures* result,
              const analysis::ConnectionPtr& ptr);
};

}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_DEBUG_H
