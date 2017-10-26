//
// Created by Arseny Tolmachev on 2017/10/13.
//

#include "feature_impl_ngram_partial.h"
#include "core_config.h"

#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
#define JPP_DO_PREFETCH(x)                                                    \
  ::jumanpp::util::prefetch<::jumanpp::util::PrefetchHint::PREFETCH_HINT_T2>( \
      (x));
#else
#define JPP_DO_PREFETCH(x)
#endif  // JPP_PREFETCH_FEATURE_WEIGHTS

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

Status PartialNgramDynamicFeatureApply::addChild(const NgramFeature& nf) {
  auto& args = nf.arguments;
  auto sz = args.size();
  switch (sz) {
    case 1: {
      unigrams_.emplace_back(unigrams_.size(), nf.index, args[0]);
      break;
    }
    case 2: {
      bigrams_.emplace_back(bigrams_.size(), nf.index, args[0], args[1]);
      break;
    }
    case 3: {
      trigrams_.emplace_back(trigrams_.size(), nf.index, args[0], args[1],
                             args[2]);
      break;
    }
    default:
      return JPPS_INVALID_STATE << "invalid ngram feature #" << nf.index
                                << "of order " << nf.arguments.size()
                                << " only 1-3 are supported";
  }
  return Status::Ok();
}

void PartialNgramDynamicFeatureApply::uniStep0(
    util::ArraySlice<u64> patterns, u32 mask, util::ArraySlice<float> weights,
    util::MutableArraySlice<u32> result) const noexcept {
  for (auto& uni : unigrams_) {
    uni.step0(patterns, result, mask);
    JPP_DO_PREFETCH(weights.data() + uni.target());
  }
}

void PartialNgramDynamicFeatureApply::biStep0(
    util::ArraySlice<u64> patterns, util::MutableArraySlice<u64> state) const
    noexcept {
  for (auto& bi : bigrams_) {
    bi.step0(patterns, state);
  }
}

void PartialNgramDynamicFeatureApply::biStep1(
    util::ArraySlice<u64> patterns, util::ArraySlice<u64> state, u32 mask,
    util::ArraySlice<float> weights, util::MutableArraySlice<u32> result) const
    noexcept {
  for (auto& bi : bigrams_) {
    auto idx = bi.step1(patterns, state, result, mask);
    JPP_DO_PREFETCH(&weights.at(idx));
  }
}

void PartialNgramDynamicFeatureApply::triStep0(
    util::ArraySlice<u64> patterns, util::MutableArraySlice<u64> state) const
    noexcept {
  for (auto& tri : trigrams_) {
    tri.step0(patterns, state);
  }
}

void PartialNgramDynamicFeatureApply::triStep1(
    util::ArraySlice<u64> patterns, util::ArraySlice<u64> state,
    util::MutableArraySlice<u64> result) const noexcept {
  for (auto& tri : trigrams_) {
    tri.step1(patterns, state, result);
  }
}

void PartialNgramDynamicFeatureApply::triStep2(
    util::ArraySlice<u64> patterns, util::ArraySlice<u64> state, u32 mask,
    util::ArraySlice<float> weights, util::MutableArraySlice<u32> result) const
    noexcept {
  for (auto& tri : trigrams_) {
    auto idx = tri.step2(patterns, state, result, mask);
    JPP_DO_PREFETCH(&weights.at(idx));
  }
}

void PartialNgramDynamicFeatureApply::biFull(
    util::ArraySlice<u64> t0, util::ArraySlice<u64> t1, u32 mask,
    util::ArraySlice<float> weights, util::MutableArraySlice<u32> result) const
    noexcept {
  for (auto& bi : bigrams_) {
    auto idx = bi.jointApply(t0, t1, result, mask);
    JPP_DO_PREFETCH(&weights.at(idx));
  }
}

void PartialNgramDynamicFeatureApply::triFull(
    util::ArraySlice<u64> t0, util::ArraySlice<u64> t1,
    util::ArraySlice<u64> t2, u32 mask, util::ArraySlice<float> weights,
    util::MutableArraySlice<u32> result) const noexcept {
  for (auto& tri : trigrams_) {
    auto idx = tri.jointApply(t0, t1, t2, result, mask);
    JPP_DO_PREFETCH(&weights.at(idx));
  }
}

bool outputBiApply2(util::io::Printer& p,
                    util::ArraySlice<BigramFeature> bigrams) {
  u32 numVars = 1;
  if (bigrams.size() >= 8) {
    numVars = 8;
  } else if (bigrams.size() >= 4) {
    numVars = 4;
  } else if (bigrams.size() >= 2) {
    numVars = 2;
  }

  p << "\n\nvoid applyBiStep2("  // args
    << JPP_TEXT(::jumanpp::core::features::FeatureBuffer*) << " buffers, "
    << JPP_TEXT(::jumanpp::util::ArraySlice<jumanpp::u64>) << " p1, "
    << JPP_TEXT(::jumanpp::core::analysis::FeatureScorer*) << " scorer, "
    << JPP_TEXT(::jumanpp::util::MutableArraySlice<float>) << " result"
    << ") const noexcept override {";
  {
    util::io::Indent fnid{p, 2};
    p << "\nauto numElems = buffers->currentElems;";
    p << "\nif (numElems == 0) { return; }";
    p << "\nauto numBigrams = this->numBigrams();";
    p << "\nauto buf1 = buffers->valBuf1(numBigrams);";
    p << "\nauto buf2 = buffers->valBuf2(numBigrams);";

    p << "\nconst auto state = buffers->t1Buf(numBigrams, numElems);";
    p << "\nauto weights = scorer->weights();";
    p << "\nauto mask = static_cast<::jumanpp::u32>(weights.size() - 1);";
    p << "\nthis->biStep1(p1, state.row(0), mask, weights, buf2);";
    p << "\n::jumanpp::u32 row = 1;";

    p << "\nfor (; row < state.numRows(); ++row) {";
    {
      util::io::Indent id2{p, 2};
      p << "\nauto srow = state.row(row);";
      for (int i = 0; i < numVars; ++i) {
        p << "\nfloat f_" << i << " = 0;";
      }
      for (int i = 0; i < bigrams.size(); ++i) {
        auto& bi = bigrams.at(i);
        p << "\n{";
        util::io::Indent id3{p, 2};
        bi.writeMember(p, i);
        p << "\nf_" << i % numVars << " += weights.at(buf2.at(" << i << "));";
        p << "\n::jumanpp::u32 idx = bi_" << i
          << ".step1(p1, srow, buf1, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
        p << "\n"
          << JPP_TEXT(::jumanpp::util::prefetch<
                      ::jumanpp::util::PrefetchHint::PREFETCH_HINT_T2>)
          << "(&weights.at(idx));";
#endif
        p << "\n}";
      }

      p << "\nresult.at(row - 1) += (f_0";
      for (int i = 1; i < numVars; ++i) {
        p << " + f_" << i;
      }
      p << ");";
      p << "\nbuf1.swap(buf2);";
    }
    p << "\n}\n";
    p << "\nresult.at(row - 1) += "
      << "::jumanpp::core::analysis::impl::computeUnrolled4RawPerceptron("
         "weights, buf2);";
  }

  p << "\n}\n";
  return true;
}

bool outputTriApply3(util::io::Printer& p,
                     util::ArraySlice<TrigramFeature> trigrams) {
  u32 numVars = 1;
  if (trigrams.size() >= 8) {
    numVars = 8;
  } else if (trigrams.size() >= 4) {
    numVars = 4;
  } else if (trigrams.size() >= 2) {
    numVars = 2;
  }

  p << "\n\nvoid applyTriStep3("  // args
    << JPP_TEXT(::jumanpp::core::features::FeatureBuffer*) << " buffers, "
    << JPP_TEXT(::jumanpp::util::ArraySlice<jumanpp::u64>) << " p2, "
    << JPP_TEXT(::jumanpp::core::analysis::FeatureScorer*) << " scorer, "
    << JPP_TEXT(::jumanpp::util::MutableArraySlice<float>) << " result"
    << ") const noexcept override {";
  {
    util::io::Indent fnid{p, 2};
    p << "\nauto numElems = buffers->currentElems;";
    p << "\nif (numElems == 0) { return; }";
    p << "\nauto numTrigrams = this->numTrigrams();";
    p << "\nauto buf1 = buffers->valBuf1(numTrigrams);";
    p << "\nauto buf2 = buffers->valBuf2(numTrigrams);";

    p << "\nconst auto state = buffers->t2Buf2(numTrigrams, numElems);";
    p << "\nauto weights = scorer->weights();";
    p << "\nauto mask = static_cast<::jumanpp::u32>(weights.size() - 1);";
    p << "\nthis->triStep2(p2, state.row(0), mask, weights, buf2);";
    p << "\n::jumanpp::u32 row = 1;";

    p << "\nfor (; row < state.numRows(); ++row) {";
    {
      util::io::Indent id2{p, 2};
      p << "\nauto srow = state.row(row);";
      for (int i = 0; i < numVars; ++i) {
        p << "\nfloat f_" << i << " = 0;";
      }
      for (int i = 0; i < trigrams.size(); ++i) {
        auto& tri = trigrams.at(i);
        p << "\n{";
        util::io::Indent id3{p, 2};
        tri.writeMember(p, i);
        p << "\nf_" << i % numVars << " += weights.at(buf2.at(" << i << "));";
        p << "\n::jumanpp::u32 idx = tri_" << i
          << ".step2(p2, srow, buf1, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
        p << "\n"
          << JPP_TEXT(::jumanpp::util::prefetch<
                      ::jumanpp::util::PrefetchHint::PREFETCH_HINT_T2>)
          << "(&weights.at(idx));";
#endif
        p << "\n}";
      }

      p << "\nresult.at(row - 1) += (f_0";
      for (int i = 1; i < numVars; ++i) {
        p << " + f_" << i;
      }
      p << ");";
      p << "\nbuf1.swap(buf2);";
    }
    p << "\n}\n";
    p << "\nresult.at(row - 1) += "
      << "::jumanpp::core::analysis::impl::computeUnrolled4RawPerceptron("
         "weights, buf2);";
  }

  p << "\n}\n";
  return true;
}

bool PartialNgramDynamicFeatureApply::outputClassBody(
    util::io::Printer& p) const {
  p << "\nvoid uniStep0("  // args
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " patterns, "
    << JPP_TEXT(jumanpp::u32) << " mask, "
    << JPP_TEXT(jumanpp::util::ArraySlice<float>) << " weights, "
    << JPP_TEXT(jumanpp::util::MutableArraySlice<jumanpp::u32>) << " result"
    << ") const noexcept {";
  {
    util::io::Indent id{p, 2};
    for (int i = 0; i < unigrams_.size(); ++i) {
      auto& f = unigrams_[i];
      f.writeMember(p, i);
      p << "\nuni_" << i << ".step0(patterns, result, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
      p << "\n"
        << JPP_TEXT(::jumanpp::util::prefetch<
                    ::jumanpp::util::PrefetchHint::PREFETCH_HINT_T2>)
        << "("
        << "&weights.at(uni_" << i << ".target()));";
#endif
    }
  }
  p << "\n}\n";

  p << "\nvoid biStep0("  // args
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " patterns, "
    << JPP_TEXT(jumanpp::util::MutableArraySlice<jumanpp::u64>) << " state"
    << ") const noexcept {";
  {
    util::io::Indent id{p, 2};
    for (int i = 0; i < bigrams_.size(); ++i) {
      auto& f = bigrams_[i];
      f.writeMember(p, i);
      p << "\nbi_" << i << ".step0(patterns, state);";
    }
  }
  p << "\n}\n";

  p << "\nvoid biStep1("  // args
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " patterns, "
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " state, "
    << JPP_TEXT(jumanpp::u32) << " mask, "
    << JPP_TEXT(jumanpp::util::ArraySlice<float>) << " weights, "
    << JPP_TEXT(jumanpp::util::MutableArraySlice<jumanpp::u32>) << " result"
    << ") const noexcept {";
  {
    util::io::Indent id{p, 2};
    for (int i = 0; i < bigrams_.size(); ++i) {
      auto& f = bigrams_[i];
      f.writeMember(p, i);
      p << "\nbi_" << i << ".step1(patterns, state, result, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
      p << "\n"
        << JPP_TEXT(::jumanpp::util::prefetch<
                    ::jumanpp::util::PrefetchHint::PREFETCH_HINT_T2>)
        << "("
        << "&weights.at(bi_" << i << ".target()));";
#endif
    }
  }
  p << "\n}\n";

  p << "\nvoid triStep0(" << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>)
    << " patterns, " << JPP_TEXT(jumanpp::util::MutableArraySlice<jumanpp::u64>)
    << " state"
    << ") const noexcept {";
  {
    util::io::Indent id{p, 2};
    for (int i = 0; i < trigrams_.size(); ++i) {
      auto& f = trigrams_[i];
      f.writeMember(p, i);
      p << "\ntri_" << i << ".step0(patterns, state);";
    }
  }
  p << "\n}\n";

  p << "\nvoid triStep1(" << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>)
    << " patterns, " << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>)
    << " state, " << JPP_TEXT(jumanpp::util::MutableArraySlice<jumanpp::u64>)
    << " result"
    << ") const noexcept {";
  {
    util::io::Indent id{p, 2};
    for (int i = 0; i < trigrams_.size(); ++i) {
      auto& f = trigrams_[i];
      f.writeMember(p, i);
      p << "\ntri_" << i << ".step1(patterns, state, result);";
    }
  }
  p << "\n}\n";

  p << "\nvoid triStep2("  // args
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " patterns, "
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " state, "
    << JPP_TEXT(jumanpp::u32) << " mask, "
    << JPP_TEXT(jumanpp::util::ArraySlice<float>) << " weights, "
    << JPP_TEXT(jumanpp::util::MutableArraySlice<jumanpp::u32>) << " result"
    << ") const noexcept {";
  {
    util::io::Indent id{p, 2};
    for (int i = 0; i < trigrams_.size(); ++i) {
      auto& f = trigrams_[i];
      f.writeMember(p, i);
      p << "\ntri_" << i << ".step2(patterns, state, result, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
      p << "\n"
        << JPP_TEXT(::jumanpp::util::prefetch<
                    ::jumanpp::util::PrefetchHint::PREFETCH_HINT_T2>)
        << "("
        << "&weights.at(tri_" << i << ".target()));";
#endif
    }
  }
  p << "\n}\n";

  p << "\nvoid biFull("  // args
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " t0, "
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " t1, "
    << JPP_TEXT(jumanpp::u32) << " mask, "
    << JPP_TEXT(jumanpp::util::ArraySlice<float>) << " weights, "
    << JPP_TEXT(jumanpp::util::MutableArraySlice<jumanpp::u32>) << " result"
    << ") const noexcept {";
  {
    util::io::Indent id{p, 2};
    for (int i = 0; i < bigrams_.size(); ++i) {
      auto& f = bigrams_[i];
      f.writeMember(p, i);
      p << "\nauto r_" << i << " = "
        << "bi_" << i << ".jointApply(t0, t1, result, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
      p << "\n"
        << JPP_TEXT(::jumanpp::util::prefetch<
                    ::jumanpp::util::PrefetchHint::PREFETCH_HINT_T2>)
        << "("
        << "&weights.at(r_" << i << "));";
#endif
    }
  }
  p << "\n}\n";

  p << "\nvoid triFull("  // args
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " t0, "
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " t1, "
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " t2, "
    << JPP_TEXT(jumanpp::u32) << " mask, "
    << JPP_TEXT(jumanpp::util::ArraySlice<float>) << " weights, "
    << JPP_TEXT(jumanpp::util::MutableArraySlice<jumanpp::u32>) << " result"
    << ") const noexcept {";
  {
    util::io::Indent id{p, 2};
    for (int i = 0; i < trigrams_.size(); ++i) {
      auto& f = trigrams_[i];
      f.writeMember(p, i);
      p << "\nauto r_" << i << " = "
        << "tri_" << i << ".jointApply(t0, t1, t2, result, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
      p << "\n"
        << JPP_TEXT(::jumanpp::util::prefetch<
                    ::jumanpp::util::PrefetchHint::PREFETCH_HINT_T2>)
        << "("
        << "&weights.at(r_" << i << "));";
#endif
    }
  }
  p << "\n}\n";

  p << "\nvoid allocateBuffers("  // args
    << JPP_TEXT(::jumanpp::core::features::FeatureBuffer*) << " fbuf,"
    << JPP_TEXT(const ::jumanpp::core::features::AnalysisRunStats&) << " stats,"
    << JPP_TEXT(::jumanpp::util::memory::ManagedAllocatorCore*) << " alloc"
    << ") const override {";
  {
    util::io::Indent id{p, 2};
    p << "\nusing namespace jumanpp;";
    p << "\nu32 maxNgrams = std::max({numUnigrams(), numBigrams(), "
         "numTrigrams()});";
    p << "\nfbuf->currentElems = ~0u;";
    p << "\nfbuf->valueBuffer1 = alloc->allocateBuf<u32>(maxNgrams, 64);";
    p << "\nfbuf->valueBuffer2 = alloc->allocateBuf<u32>(maxNgrams, 64);";
    p << "\nfbuf->t1Buffer = alloc->allocateBuf<u64>(numBigrams() * "
         "stats.maxStarts, 64);";
    p << "\nfbuf->t2Buffer1 = alloc->allocateBuf<u64>(numTrigrams() * "
         "stats.maxStarts, 64);";
    p << "\nfbuf->t2Buffer2 = alloc->allocateBuf<u64>(numTrigrams() * "
         "stats.maxStarts, 64);";
    p << "\nfbuf->scoreBuffer = alloc->allocateBuf<float>(stats.maxEnds, 16);";
  }
  p << "\n}\n";

  p << "\n::jumanpp::u32 numUnigrams() const noexcept { "
    << "return " << this->numUnigrams() << "; }";

  p << "\n::jumanpp::u32 numBigrams() const noexcept { "
    << "return " << this->numBigrams() << "; }";

  p << "\n::jumanpp::u32 numTrigrams() const noexcept { "
    << "return " << this->numTrigrams() << "; }";

  outputBiApply2(p, this->bigrams_);
  outputTriApply3(p, this->trigrams_);

  return true;
}

void PartialNgramDynamicFeatureApply::allocateBuffers(
    FeatureBuffer* buffer, const AnalysisRunStats& stats,
    util::memory::ManagedAllocatorCore* alloc) const {
  u32 maxNgrams = std::max({numUnigrams(), numBigrams(), numTrigrams()});
  buffer->currentElems = ~0u;
  buffer->valueBuffer1 = alloc->allocateBuf<u32>(maxNgrams, 64);
  buffer->valueBuffer2 = alloc->allocateBuf<u32>(maxNgrams, 64);
  buffer->t1Buffer =
      alloc->allocateBuf<u64>(numBigrams() * stats.maxStarts, 64);
  buffer->t2Buffer1 =
      alloc->allocateBuf<u64>(numTrigrams() * stats.maxStarts, 64);
  buffer->t2Buffer2 =
      alloc->allocateBuf<u64>(numTrigrams() * stats.maxStarts, 64);
  buffer->scoreBuffer = alloc->allocateBuf<float>(stats.maxEnds, 16);
}

void UnigramFeature::writeMember(util::io::Printer& p, i32 count) const {
  p << "\nconstexpr const "
    << "::jumanpp::core::features::impl::" << JPP_TEXT(UnigramFeature)
    << " uni_" << count << " { " << target_ << ", " << index_ << ", " << t0idx_
    << " };";
}

void BigramFeature::writeMember(util::io::Printer& p, i32 count) const {
  p << "\nconstexpr const "
    << "::jumanpp::core::features::impl::" << JPP_TEXT(BigramFeature) << " bi_"
    << count << " { " << target_ << ", " << index_ << ", " << t0idx_ << ", "
    << t1idx_ << " };";
}

void TrigramFeature::writeMember(util::io::Printer& p, i32 count) const {
  p << "\nconstexpr const "
    << "::jumanpp::core::features::impl::" << JPP_TEXT(TrigramFeature)
    << " tri_" << count << " { " << target_ << ", " << index_ << ", " << t0idx_
    << ", " << t1idx_ << ", " << t2idx_ << " };";
}
}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp