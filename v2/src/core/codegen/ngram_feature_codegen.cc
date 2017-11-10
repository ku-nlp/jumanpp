//
// Created by Arseny Tolmachev on 2017/11/10.
//

#include "core/codegen/ngram_feature_codegen.h"
#include "core/codegen/pattern_feature_codegen.h"
#include "core/impl/feature_impl_ngram_partial.h"

namespace jumanpp {
namespace core {
namespace codegen {

NgramFeatureCodegen::NgramFeatureCodegen(
    const spec::NgramFeatureDescriptor &ngram, const spec::AnalysisSpec &spec)
    : ngram_(ngram), target_{0}, name_{"fng_"} {
  switch (ngram_.references.size()) {
    case 1:
      name_ += "uni";
      break;
    case 2:
      name_ += "bi";
      break;
    case 3:
      name_ += "tri";
      break;
    default:
      name_ += "wtf_";
      name_ += std::to_string(ngram_.references.size());
  }
  name_ += "_";
  name_ += std::to_string(ngram.index);
  name_ += "_";

  for (auto &nf : spec.features.ngram) {
    if (nf.index == ngram.index) {
      break;
    }
    if (nf.references.size() == ngram.references.size()) {
      target_ += 1;
    }
  }
}

void NgramFeatureCodegen::makePartialObject(util::io::Printer &p) const {
  p << "\nconstexpr ";
  switch (ngram_.references.size()) {
    case 1:
      p << JPP_TEXT(::jumanpp::core::features::impl::UnigramFeature) << " "
        << name_ << "{" << target_ << ", " << ngram_.index << ", "
        << ngram_.references[0] << "};";
      break;
    case 2:
      p << JPP_TEXT(::jumanpp::core::features::impl::BigramFeature) << " "
        << name_ << "{" << target_ << ", " << ngram_.index << ", "
        << ngram_.references[0] << ", " << ngram_.references[1] << "};";
      break;
    case 3:
      p << JPP_TEXT(::jumanpp::core::features::impl::TrigramFeature) << " "
        << name_ << "{" << target_ << ", " << ngram_.index << ", "
        << ngram_.references[0] << ", " << ngram_.references[1] << ", "
        << ngram_.references[2] << "};";
      break;
    default:
      throw std::runtime_error("invalid ngram order");
  }
}

void NgramFeatureCodegen::biStep0(i::Printer &p, StringPiece patterns,
                                  StringPiece state) const {
  p << "\n" << name_ << ".step0(" << patterns << ", " << state << ");";
}

void NgramFeatureCodegen::triStep0(i::Printer &p, StringPiece patterns,
                                   StringPiece state) const {
  biStep0(p, patterns, state);
}

std::string NgramFeatureCodegen::rawUniStep0(i::Printer &p, StringPiece t0,
                                             StringPiece mask) const {
  std::string result = concat("value_", name_);
  p << "\nauto " << result << " = " << name_ << ".maskedValueFor(" << t0 << ", "
    << mask << ");";
  return result;
}

}  // namespace codegen
}  // namespace core
}  // namespace jumanpp