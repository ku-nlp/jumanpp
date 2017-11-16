//
// Created by Arseny Tolmachev on 2017/11/11.
//

#include "partial_ngram_feature_codegen.h"
#include "core_config.h"
#include "pattern_feature_codegen.h"

namespace jumanpp {
namespace core {
namespace codegen {

inline bool outputBiApply2(util::io::Printer& p,
                           util::ArraySlice<NgramFeatureCodegen> bigrams) {
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
        bi.makePartialObject(p);
        p << "\nf_" << i % numVars << " += weights.at(buf2.at(" << i << "));";
        p << "\n::jumanpp::u32 idx = " << bi.name()
          << ".step1(p1, srow, buf1, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
        p << "\nweights.prefetch<"
          << JPP_TEXT(::jumanpp::util::PrefetchHint::PREFETCH_HINT_T0)
          << ">(idx);";
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
                     util::ArraySlice<NgramFeatureCodegen> trigrams) {
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
        tri.makePartialObject(p);
        p << "\nf_" << i % numVars << " += weights.at(buf2.at(" << i << "));";
        p << "\n::jumanpp::u32 idx = " << tri.name()
          << ".step2(p2, srow, buf1, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
        p << "\nweights.prefetch<"
          << JPP_TEXT(::jumanpp::util::PrefetchHint::PREFETCH_HINT_T0)
          << ">(idx);";
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

class VarNamer {
  size_t numVars_ = 1;

 public:
  VarNamer(size_t totalSize, size_t max = 4) {
    if (max >= 8 && totalSize >= 16) {
      numVars_ = 8;
    } else if (max >= 4 && totalSize >= 8) {
      numVars_ = 4;
    } else if (max >= 2 && totalSize >= 4) {
      numVars_ = 2;
    }
  }

  std::string nameEqOrAdd(int i) {
    return concat((i >= numVars_ ? "" : "auto "), name(i),
                  (i < numVars_ ? " = " : " += "));
  }

  std::string name(int i) { return concat("var_", i % numVars_); }

  void printSum(i::Printer& p) {
    p << "var_0";
    for (int i = 1; i < numVars_; ++i) {
      p << " + var_" << i;
    }
  }
};

void PartialNgramPrinter::outputApplyBiTriLoop(util::io::Printer& p) {
  p << "\n\nvoid applyBiTri("  // args
    << JPP_TEXT(::jumanpp::core::features::FeatureBuffer*) << " buffers, \n"
    << JPP_TEXT(::jumanpp::u32) << " t0idx, \n"
    << JPP_TEXT(::jumanpp::util::ArraySlice<jumanpp::u64>) << " t0, \n"
    << JPP_TEXT(::jumanpp::util::ConstSliceable<jumanpp::u64>) << " t1, \n"
    << JPP_TEXT(::jumanpp::util::ConstSliceable<jumanpp::u64>) << " t2, \n"
    << JPP_TEXT(::jumanpp::util::ArraySlice<jumanpp::u32>) << " t1idxes, \n"
    << JPP_TEXT(::jumanpp::core::analysis::FeatureScorer*) << " scorer, \n"
    << JPP_TEXT(::jumanpp::util::MutableArraySlice<float>) << " result"
    << ") const noexcept override {";

  {
    i::Indent id{p, 2};
    p << "\nconstexpr auto numBigrams = " << bigrams_.size() << ";";
    p << "\nconstexpr auto numTrigrams = " << trigrams_.size() << ";";
    p << "\nauto weights = scorer->weights();";
    p << "\nauto scbuf = buffers->scoreBuf(t1.numRows());";
    p << "\nconst auto bistateBuf = buffers->t1Buf(numBigrams, buffers->currentElems);";
    p << "\nconst auto tristateBuf = buffers->t2Buf1(numTrigrams, buffers->currentElems);";

    p << "\nauto buf1 = buffers->valBuf1(numBigrams);";
    p << "\nauto buf2 = buffers->valBuf2(numBigrams);";
    p << "\nstatic constexpr jumanpp::u32"
      << " t1BiFeatureIdxes[numBigrams] = {\n  ";
    for (auto& bi : bigrams_) {
      p << bi.spec().references[1] << ", ";
    }
    p << "\n};";

    p << "\nstatic constexpr jumanpp::u32"
      << " t1TriFeatureIdxes[numTrigrams] = {\n  ";
    for (auto& tri : trigrams_) {
      p << tri.spec().references[1] << ", ";
    }
    p << "\n};";
    p << "\nstatic constexpr jumanpp::u32"
      << " t2TriFeatureIdxes[numTrigrams] = {\n  ";
    for (auto& tri : trigrams_) {
      p << tri.spec().references[2] << ", ";
    }
    p << "\n};";

    p << "\n::jumanpp::core::features::impl::applyBiTriFullKernel(";
    {
      i::Indent id2{p, 2};
      p << "\nbistateBuf.row(t0idx),";
      p << "\ntristateBuf.row(t0idx),";
      p << "\nt1,";
      p << "\nt2,";
      p << "\nt1idxes,";
      p << "\nt1BiFeatureIdxes,";
      p << "\nt1TriFeatureIdxes,";
      p << "\nt2TriFeatureIdxes,";
      p << "\nbuf1,";
      p << "\nbuf2,";
      p << "\nweights,";
      p << "\nscbuf,";
      p << "\nresult";
    }
    p << "\n);";
  }

  p << "\n}";
}

void PartialNgramPrinter::outputApplyBiTriFullUnrolled(util::io::Printer& p) {
  p << "\n\nvoid applyBiTri("  // args
    << JPP_TEXT(::jumanpp::core::features::FeatureBuffer*) << " buffers, \n"
    << JPP_TEXT(::jumanpp::u32) << " t0idx, \n"
    << JPP_TEXT(::jumanpp::util::ArraySlice<jumanpp::u64>) << " t0, \n"
    << JPP_TEXT(::jumanpp::util::ConstSliceable<jumanpp::u64>) << " t1, \n"
    << JPP_TEXT(::jumanpp::util::ConstSliceable<jumanpp::u64>) << " t2, \n"
    << JPP_TEXT(::jumanpp::util::ArraySlice<jumanpp::u32>) << " t1idxes, \n"
    << JPP_TEXT(::jumanpp::core::analysis::FeatureScorer*) << " scorer, \n"
    << JPP_TEXT(::jumanpp::util::MutableArraySlice<float>) << " result"
    << ") const noexcept override {";
  {
    i::Indent id{p, 2};
    p << "\nauto numElems = t2.numRows();";
    p << "\nauto numBigrams = this->numBigrams();";
    p << "\nauto numTrigrams = this->numTrigrams();";
    p << "\nauto weights = scorer->weights();";
    p << "\nauto scbuf = buffers->scoreBuf(t1.numRows());";
    p << "\nauto mask = static_cast<::jumanpp::u32>(weights.size() - 1);";
    p << "\nauto bistateBuf = buffers->t1Buf(numBigrams, numElems);";
    p << "\nauto tristateBuf = buffers->t2Buf1(numTrigrams, numElems);";

    p << "\nauto bistates = bistateBuf.row(t0idx);";
    p << "\nauto buf1 = buffers->valBuf1(numBigrams);";
    p << "\nauto buf2 = buffers->valBuf2(numBigrams);";
    p << "\nfor (auto row = 0; row < t1.numRows(); ++row) {";
    {
      VarNamer biNames{bigrams_.size()};
      i::Indent id2{p, 2};
      p << "\nauto t1row = t1.row(row);";
      p << "\nbuf1.swap(buf2);";
      for (int i = 0; i < bigrams_.size(); ++i) {
        auto& b = bigrams_[i];
        b.makePartialObject(p);
        p << "\nauto bi_v_" << i << " = " << b.name() << ".raw2(bistates.at("
          << i << "), t1row, mask);";
        p << "\n"
          << biNames.nameEqOrAdd(i) << "weights.at(buf2.at(" << i << "));";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
        p << "\nweights.prefetch<"
          << JPP_TEXT(::jumanpp::util::PrefetchHint::PREFETCH_HINT_T0)
          << ">(bi_v_" << i << ");";
#endif
        p << "\nbuf1.at(" << i << ") = bi_v_" << i << ";";
      }
      p << "\nif (JPP_LIKELY(row > 0)) {";
      p << "\n  scbuf.at(row - 1) = ";
      biNames.printSum(p);
      p << ";\n}";
    }
    p << "\n}";

    // after the tribuf swap, buffer numbers WIIL be the same
    // and the cycle will write to tribuf1(buf2).
    // We need to keep the state of buf1
    // till the computation of the last value for the bigram loop.
    p << "\nauto tristates = tristateBuf.row(t0idx);";
    p << "\n::jumanpp::util::MutableArraySlice<::jumanpp::u32>"
      << "tribuf1{buf1.data(), numTrigrams};";
    p << "\n::jumanpp::util::MutableArraySlice<::jumanpp::u32>"
      << "tribuf2{buf2.data(), numTrigrams};";
    p << "\nfor (auto row = 0; row < t2.numRows(); ++row) {";
    {
      VarNamer triNames{trigrams_.size()};
      i::Indent id2{p, 2};
      p << "\nauto t2row = t2.row(row);";
      p << "\nauto t1row = t1.row(t1idxes.at(row));";
      p << "\ntribuf1.swap(tribuf2);";
      for (int i = 0; i < trigrams_.size(); ++i) {
        auto& b = trigrams_[i];
        b.makePartialObject(p);
        p << "\nauto tri_v_" << i << " = " << b.name() << ".raw23(tristates.at("
          << i << "), t1row, t2row, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
        p << "\nweights.prefetch<"
          << JPP_TEXT(::jumanpp::util::PrefetchHint::PREFETCH_HINT_T0)
          << ">(tri_v_" << i << ");";
#endif
        p << "\ntribuf1.at(" << i << ") = tri_v_" << i << ";";
        p << "\n"
          << triNames.nameEqOrAdd(i) << "weights.at(tribuf2.at(" << i << "));";
      }
      p << "\nif (JPP_LIKELY(row > 0)) {";
      p << "\n  result.at(row) = scbuf.at(t1idxes.at(row)) + ";
      triNames.printSum(p);
      p << ";\n} else {";
      p << "\n  scbuf.at(t1.numRows() - 1) = "
        << JPP_TEXT(
               ::jumanpp::core::analysis::impl::computeUnrolled4RawPerceptron)
        << "(weights, buf1);\n}";
    }
    p << "\n}";
  }
  p << "\nauto lastRow = t2.numRows() - 1;";
  p << "\nresult.at(lastRow) = scbuf.at(t1idxes.at(lastRow)) + "
    << JPP_TEXT(::jumanpp::core::analysis::impl::computeUnrolled4RawPerceptron)
    << "(weights, tribuf1);";

  p << "\n}";
}

void PartialNgramPrinter::outputClassBody(util::io::Printer& p) {
  p << "\nvoid uniStep0("  // args
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " patterns, "
    << JPP_TEXT(jumanpp::u32) << " mask, "
    << JPP_TEXT(::jumanpp::core::analysis::WeightBuffer) << " weights, "
    << JPP_TEXT(jumanpp::util::MutableArraySlice<jumanpp::u32>) << " result"
    << ") const noexcept {";
  {
    util::io::Indent id{p, 2};
    for (int i = 0; i < unigrams_.size(); ++i) {
      auto& f = unigrams_[i];
      f.makePartialObject(p);
      p << "\nauto uni_v_" << i << " = " << f.name()
        << ".step0(patterns, result, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
      p << "\nweights.prefetch<"
        << JPP_TEXT(::jumanpp::util::PrefetchHint::PREFETCH_HINT_T0)
        << ">(uni_v_" << i << ");";
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
      f.makePartialObject(p);
      p << "\n" << f.name() << ".step0(patterns, state);";
    }
  }
  p << "\n}\n";

  p << "\nvoid biStep1("  // args
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " patterns, "
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " state, "
    << JPP_TEXT(jumanpp::u32) << " mask, "
    << JPP_TEXT(jumanpp::core::analysis::WeightBuffer) << " weights, "
    << JPP_TEXT(jumanpp::util::MutableArraySlice<jumanpp::u32>) << " result"
    << ") const noexcept {";
  {
    util::io::Indent id{p, 2};
    for (int i = 0; i < bigrams_.size(); ++i) {
      auto& f = bigrams_[i];
      f.makePartialObject(p);
      p << "\nauto bi_val_" << i << " = " << f.name()
        << ".step1(patterns, state, result, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
      p << "\nweights.prefetch<"
        << JPP_TEXT(::jumanpp::util::PrefetchHint::PREFETCH_HINT_T0)
        << ">(bi_val_" << i << ");";
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
      f.makePartialObject(p);
      p << "\n" << f.name() << ".step0(patterns, state);";
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
      f.makePartialObject(p);
      p << "\n" << f.name() << ".step1(patterns, state, result);";
    }
  }
  p << "\n}\n";

  p << "\nvoid triStep2("  // args
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " patterns, "
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " state, "
    << JPP_TEXT(jumanpp::u32) << " mask, "
    << JPP_TEXT(jumanpp::core::analysis::WeightBuffer) << " weights, "
    << JPP_TEXT(jumanpp::util::MutableArraySlice<jumanpp::u32>) << " result"
    << ") const noexcept {";
  {
    i::Indent id{p, 2};
    for (int i = 0; i < trigrams_.size(); ++i) {
      auto& f = trigrams_[i];
      f.makePartialObject(p);
      p << "\nauto tri_v_" << i << " = " << f.name()
        << ".step2(patterns, state, result, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
      p << "\nweights.prefetch<"
        << JPP_TEXT(::jumanpp::util::PrefetchHint::PREFETCH_HINT_T0)
        << ">(tri_v_" << i << ");";
#endif
    }
  }
  p << "\n}\n";

  p << "\nvoid biFull("  // args
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " t0, "
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " t1, "
    << JPP_TEXT(jumanpp::u32) << " mask, "
    << JPP_TEXT(jumanpp::core::analysis::WeightBuffer) << " weights, "
    << JPP_TEXT(jumanpp::util::MutableArraySlice<jumanpp::u32>) << " result"
    << ") const noexcept {";
  {
    util::io::Indent id{p, 2};
    for (int i = 0; i < bigrams_.size(); ++i) {
      auto& f = bigrams_[i];
      f.makePartialObject(p);
      p << "\nauto r_" << i << " = " << f.name()
        << ".jointApply(t0, t1, result, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
      p << "\nweights.prefetch<"
        << JPP_TEXT(::jumanpp::util::PrefetchHint::PREFETCH_HINT_T0) << ">(r_"
        << i << ");";
#endif
    }
  }
  p << "\n}\n";

  p << "\nvoid triFull("  // args
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " t0, \n"
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " t1, \n"
    << JPP_TEXT(jumanpp::util::ArraySlice<jumanpp::u64>) << " t2, \n"
    << JPP_TEXT(jumanpp::u32) << " mask, \n"
    << JPP_TEXT(jumanpp::core::analysis::WeightBuffer) << " weights, \n"
    << JPP_TEXT(jumanpp::util::MutableArraySlice<jumanpp::u32>) << " result\n"
    << ") const noexcept {";
  {
    util::io::Indent id{p, 2};
    for (int i = 0; i < trigrams_.size(); ++i) {
      auto& f = trigrams_[i];
      f.makePartialObject(p);
      p << "\nauto r_" << i << " = " << f.name()
        << ".jointApply(t0, t1, t2, result, mask);";
#ifdef JPP_PREFETCH_FEATURE_WEIGHTS
      p << "\nweights.prefetch<"
        << JPP_TEXT(::jumanpp::util::PrefetchHint::PREFETCH_HINT_T0) << ">(r_"
        << i << ");";
#endif
    }
  }
  p << "\n}\n";

  p << "\nvoid allocateBuffers("  // args
    << JPP_TEXT(::jumanpp::core::features::FeatureBuffer*) << " fbuf,"
    << JPP_TEXT(const ::jumanpp::core::features::AnalysisRunStats&) << " stats,"
    << JPP_TEXT(::jumanpp::util::memory::PoolAlloc*) << " alloc"
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
    << "return " << this->unigrams_.size() << "; }";

  p << "\n::jumanpp::u32 numBigrams() const noexcept { "
    << "return " << this->bigrams_.size() << "; }";

  p << "\n::jumanpp::u32 numTrigrams() const noexcept { "
    << "return " << this->trigrams_.size() << "; }";

  outputBiApply2(p, this->bigrams_);
  outputTriApply3(p, this->trigrams_);
  outputApplyBiTriLoop(p);
}

PartialNgramPrinter::PartialNgramPrinter(const spec::AnalysisSpec& spec)
    : spec_{spec} {
  for (auto& ng : spec.features.ngram) {
    auto sz = ng.references.size();
    switch (sz) {
      case 1:
        unigrams_.emplace_back(ng, spec);
        break;
      case 2:
        bigrams_.emplace_back(ng, spec);
        break;
      case 3:
        trigrams_.emplace_back(ng, spec);
        break;
      default:
        throw std::runtime_error("ngram size was not 1,2,3");
    }
  }
}

}  // namespace codegen
}  // namespace core
}  // namespace jumanpp