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
  i32 globalBeamSize = 0;
  i32 otherScorersTopN = 0;
  i32 rightGbeamCheck = 0;
  i32 rightGbeamSize = 0;
  bool storeAllPatterns = false;
  i32 autoBeamStep = 0;
  i32 autoBeamBase = 0;
  i32 autoBeamMax = 0;
};

/**
 * Analyser depends on all core, so put it in pimpl idiom to reduce header
 * overload.
 * It has only high-level APIs, so we do not care about +1 level of indirection
 */
class AnalyzerImpl;

class ScorePlugin;

struct ScorerDef;

class Analyzer {
  std::unique_ptr<AnalyzerImpl> pimpl_;
  AnalyzerImpl* ptr_;
  const ScorerDef* scorer_;

 public:
  Analyzer();
  Analyzer(const Analyzer&) = delete;
  Analyzer(Analyzer&&) = delete;

  Status initialize(const CoreHolder* core, const AnalyzerConfig& cfg,
                    const ScoringConfig& sconf, const ScorerDef* scorer);
  Status initialize(AnalyzerImpl* impl, const ScorerDef* scorer);
  Status analyze(StringPiece input, ScorePlugin* plugin = nullptr);
  const OutputManager& output() const;

  const ScorerDef* scorer() const { return scorer_; }
  AnalyzerImpl* impl() const { return ptr_; }
  ~Analyzer();
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_ANALYZER_H
