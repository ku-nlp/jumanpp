//
// Created by Arseny Tolmachev on 2017/11/11.
//

#ifndef JUMANPP_PARTIAL_NGRAM_FEATURE_CODEGEN_H
#define JUMANPP_PARTIAL_NGRAM_FEATURE_CODEGEN_H

#include "core/codegen/ngram_feature_codegen.h"

namespace jumanpp {
namespace core {
namespace codegen {

class PartialNgramPrinter {
  const spec::AnalysisSpec& spec_;
  std::vector<NgramFeatureCodegen> unigrams_;
  std::vector<NgramFeatureCodegen> bigrams_;
  std::vector<NgramFeatureCodegen> trigrams_;

 public:
  PartialNgramPrinter(const spec::AnalysisSpec& spec);
  void outputClassBody(util::io::Printer& p);
  void outputApplyBiTri(util::io::Printer& p);
};

}  // namespace codegen
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_PARTIAL_NGRAM_FEATURE_CODEGEN_H
