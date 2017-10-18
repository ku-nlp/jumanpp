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
#define JPP_DO_PREFETCH(x) ()
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
    bi.step1(patterns, state, result, mask);
    JPP_DO_PREFETCH(weights.data() + bi.target());
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
    tri.step2(patterns, state, result, mask);
    JPP_DO_PREFETCH(weights.data() + tri.target());
  }
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
  }

  p << "\n}\n";

  p << "\n::jumanpp::u32 numUnigrams() const noexcept { "
    << "return " << this->numUnigrams() << "; }";

  p << "\n::jumanpp::u32 numBigrams() const noexcept { "
    << "return " << this->numBigrams() << "; }";

  p << "\n::jumanpp::u32 numTrigrams() const noexcept { "
    << "return " << this->numTrigrams() << "; }";

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