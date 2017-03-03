//
// Created by Arseny Tolmachev on 2017/03/02.
//

#include "core/analysis/analyzer.h"
#include "core/analysis/analyzer_impl.h"

namespace jumanpp {
namespace core {
namespace analysis {

Analyzer::Analyzer(const CoreHolder *core, const AnalyzerConfig &cfg)
    : pimpl_{new AnalyzerImpl{core, cfg}} {}

Analyzer::~Analyzer() {}

}  // analysis
}  // core
}  // jumanpp
