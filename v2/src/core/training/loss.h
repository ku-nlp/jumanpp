//
// Created by Arseny Tolmachev on 2017/03/23.
//

#ifndef JUMANPP_LOSS_H
#define JUMANPP_LOSS_H

#include "core/analysis/analysis_result.h"
#include "core/analysis/analyzer_impl.h"
#include "training_io.h"

namespace jumanpp {
namespace core {
namespace training {

class LossCalculator {
  analysis::AnalysisPath top1;
  spec::TrainingSpec* spec;
  std::vector<u32> top1Features;

 public:
  double fullExampleLoss(const FullyAnnotatedExample& ex,
                         analysis::AnalyzerImpl* analyzer) {
    if (!top1.fillIn(analyzer->lattice())) {
      return std::numeric_limits<double>::signaling_NaN();
    }

    double result = 0;

    while (top1.nextBoundary()) {
    }
  }
};

}  // training
}  // core
}  // jumanpp

#endif  // JUMANPP_LOSS_H
