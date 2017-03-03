//
// Created by Arseny Tolmachev on 2017/03/02.
//

#ifndef JUMANPP_ANALYZER_H
#define JUMANPP_ANALYZER_H

#include "core/analysis/output.h"
#include "core/core.h"

namespace jumanpp {
namespace core {
namespace analysis {

struct AnalyzerConfig {
  size_t pageSize = 4 * 1024 * 1024;
  size_t maxInputBytes = 4 * 1024;
};

/**
 * Analyser depends on all core, so put it in pimpl idiom to reduce header
 * overload.
 * It has only high-level APIs, so we do not care about +1 level of indirection
 */
class AnalyzerImpl;

class Analyzer {
  std::unique_ptr<AnalyzerImpl> pimpl_;

 public:
  Analyzer(const Analyzer&) = delete;
  Analyzer(Analyzer&&) = delete;

  Analyzer(const CoreHolder* core, const AnalyzerConfig& cfg);
  ~Analyzer();
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_ANALYZER_H
