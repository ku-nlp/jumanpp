//
// Created by Arseny Tolmachev on 2017/11/09.
//

#include "core/codegen/pattern_feature_codegen.h"
#include "core/codegen/ngram_feature_codegen.h"
#include "core/features_api.h"
#include "core/impl/feature_impl_types.h"

namespace jumanpp {
namespace core {
namespace codegen {

std::string InNodeComputationsCodegen::patternValue(
    i::Printer &p, const spec::PatternFeatureDescriptor &pat) {
  for (auto ref : pat.references) {
    ensureCompute(p, spec_.features.computation[ref]);
  }
  std::string name = concat("fe_pat_", pat.index);
  std::string hashName = concat("fe_pat_hash_", pat.index);
  p << "\nauto " << hashName << " = ::jumanpp::util::hashing::FastHash1{}.mix("
    << pat.index << "ULL).mix(" << pat.references.size() << "ULL).mix("
    << features::impl::PatternFeatureSeed << "ULL);";
  for (auto ref : pat.references) {
    auto &compF = spec_.features.computation[ref];
    printCompute(p, compF, hashName);
  }
  p << "\n::jumanpp::u64 " << name << " = " << hashName << ".result();";
  return name;
}

void InNodeComputationsCodegen::printCompute(
    i::Printer &p, const spec::ComputationFeatureDescriptor &cfd,
    StringPiece patternName) {
  if (cfd.falseBranch.empty() && cfd.trueBranch.empty()) {
    p << "\n"
      << patternName << " = " << patternName << ".mix("
      << primitiveNames_[cfd.primitiveFeature] << ");";
  } else {
    p << "\nif (" << primitiveNames_[cfd.primitiveFeature] << " != 0) {";
    {
      p << " // compute feature " << cfd.name;
      i::Indent id{p, 2};
      p << "\n" << patternName << " = " << patternName;
      for (auto pf : cfd.trueBranch) {
        p << ".mix(" << primitiveNames_[pf] << ")";
      }
    }
    p << ";\n} else {";
    {
      i::Indent id{p, 2};
      p << "\n" << patternName << " = " << patternName;
      for (auto pf : cfd.falseBranch) {
        p << ".mix(" << primitiveNames_[pf] << ")";
      }
    }
    p << ";\n}";
  }
}

void InNodeComputationsCodegen::ensureCompute(
    i::Printer &p, const spec::ComputationFeatureDescriptor &cfd) {
  ensurePrimitive(p, cfd.primitiveFeature);
  for (auto pf : cfd.trueBranch) {
    ensurePrimitive(p, pf);
  }
  for (auto pf : cfd.falseBranch) {
    ensurePrimitive(p, pf);
  }
}

void InNodeComputationsCodegen::ensurePrimitive(i::Printer &p,
                                                i32 primitiveIdx) {
  auto it = primitiveNames_.find(primitiveIdx);
  if (it != primitiveNames_.end()) {
    return;
  }

  auto &pf = spec_.features.primitive[primitiveIdx];
  auto objName = primFeatureObjectName(p, pf);
  std::string name = concat("pf_", pf.name, "_", primitiveNames_.size());
  p << "\n::jumanpp::u64 " << name << " = " << objName
    << ".access(ctx, nodeInfo, entry);";
  primitiveNames_[pf.index] = name;
}

std::string InNodeComputationsCodegen::primFeatureObjectName(
    i::Printer &p, const spec::PrimitiveFeatureDescriptor &pfd) {
  std::string name = concat("pfobj_", pfd.name, "_");
  JPP_CAPTURE(pfd.name);
  using namespace jumanpp::core::features::impl;
  p << "\nconstexpr jumanpp::core::features::impl::";
  switch (pfd.kind) {
    case spec::PrimitiveFeatureKind::Invalid:
      throw std::runtime_error("got an invalid primitive feature type");
    case spec::PrimitiveFeatureKind::Copy:
      JPP_DCHECK_EQ(pfd.references.size(), 1);
      p << JPP_TEXT(CopyPrimFeatureImpl) << " " << name << "{"
        << pfd.references[0] << "};";
      break;
    case spec::PrimitiveFeatureKind::SingleBit:
      JPP_DCHECK_EQ(pfd.references.size(), 2);
      p << JPP_TEXT(ShiftMaskPrimFeatureImpl) << " " << name << "{"
        << pfd.references[0] << ", " << pfd.references[1] << ", 0x1};";
      break;
    case spec::PrimitiveFeatureKind::Provided:
      JPP_DCHECK_EQ(pfd.references.size(), 1);
      p << JPP_TEXT(ProvidedPrimFeatureImpl) << " " << name << "{"
        << pfd.references[0] << "};";
      break;
    case spec::PrimitiveFeatureKind::ByteLength:
      JPP_DCHECK_EQ(pfd.references.size(), 1);
      p << JPP_TEXT(ByteLengthPrimFeatureImpl) << " " << name << "{"
        << pfd.references[0] << ", " << lengthTypeOf(pfd.references[0]) << "};";
      break;
    case spec::PrimitiveFeatureKind::CodepointSize:
      JPP_DCHECK_EQ(pfd.references.size(), 1);
      p << JPP_TEXT(CodepointLengthPrimFeatureImpl) << " " << name << "{"
        << pfd.references[0] << ", " << lengthTypeOf(pfd.references[0]) << "};";
      break;
    case spec::PrimitiveFeatureKind::SurfaceCodepointSize:
      JPP_DCHECK_EQ(pfd.references.size(), 0);
      p << JPP_TEXT(SurfaceCodepointLengthPrimFeatureImpl) << " " << name
        << "{};";
      break;
    case spec::PrimitiveFeatureKind::Codepoint:
      JPP_DCHECK_EQ(pfd.references.size(), 1);
      p << JPP_TEXT(CodepointFeatureImpl) << " " << name << "{"
        << pfd.references[0] << "};";
      break;
    case spec::PrimitiveFeatureKind::CodepointType:
      JPP_DCHECK_EQ(pfd.references.size(), 1);
      p << JPP_TEXT(CodepointTypeFeatureImpl) << " " << name << "{"
        << pfd.references[0] << "};";
      break;
  }
  return name;
}

StringPiece InNodeComputationsCodegen::lengthTypeOf(i32 dicField) const {
  for (auto &fld : spec_.dictionary.fields) {
    if (fld.dicIndex == dicField) {
      switch (fld.fieldType) {
        case spec::FieldType::String:
          return StringPiece{
              "::jumanpp::core::features::impl::LengthFieldSource::Strings"};
        case spec::FieldType::StringList:
        case spec::FieldType::StringKVList:
          return StringPiece{
              "::jumanpp::core::features::impl::LengthFieldSource::Positions"};
        default:
          break;
      }
    }
  }
  return StringPiece{
      "::jumanpp::core::features::impl::LengthFieldSource::Invalid"};
}

void InNodeComputationsCodegen::generate(i::Printer &p, StringPiece className) {
  p << "\nclass " << className << " : public "
    << JPP_TEXT(::jumanpp::core::features::GeneratedPatternFeatureApply)
    << " {";
  {
    i::Indent id{p, 2};
    p << "\nvoid patternsAndUnigramsApply(";
    p.addIndent(8);
    p << "\n"
      << JPP_TEXT(::jumanpp::core::features::impl::PrimitiveFeatureContext)
      << " *ctx,";
    p << "\n"
      << JPP_TEXT(::jumanpp::util::ArraySlice<::jumanpp::core::NodeInfo>)
      << " nodeInfos,";
    p << "\n"
      << JPP_TEXT(::jumanpp::core::features::FeatureBuffer) << " *fbuffer,";
    p << "\n"
      << JPP_TEXT(::jumanpp::util::Sliceable<::jumanpp::u64>)
      << " patternMatrix,";
    p << "\nconst " << JPP_TEXT(::jumanpp::core::analysis::FeatureScorer)
      << " *scorer,";
    p << "\n"
      << JPP_TEXT(::jumanpp::util::MutableArraySlice<float>) << " scores";
    p.addIndent(-8);

    p << ") const override {";
    {
      i::Indent id2{p, 2};
      generateLoop(p);
    }
    p << "\n} //end function";
  }

  p << "\n}; // class " << className;
}

void InNodeComputationsCodegen::generateLoopBody(i::Printer &p) {
  std::vector<i32> hasUniPattern;

  auto numUniPats = numUnigrams_;

  int numVars = 1;
  if (numUniPats >= 8) {
    numVars = 4;
  } else if (numUniPats >= 4) {
    numVars = 2;
  }

  // first compute unigram scores + output features used by non-unigrams
  int varUsage = 0;
  for (auto &uni : spec_.features.ngram) {
    if (uni.references.size() != 1) {
      continue;
      // only unigrams may pass
    }

    auto &pat = spec_.features.pattern[uni.references[0]];
    p << "\n// pattern feature #" << pat.index
      << " (with unigram), usage=" << pat.usage;

    auto name = patternValue(p, pat);
    NgramFeatureCodegen ngramCodegen{uni, spec_};
    ngramCodegen.makePartialObject(p);
    auto uniVal = ngramCodegen.rawUniStep0(p, name, "mask");

    if (varUsage < numVars) {
      p << "\nfloat score_part_" << (varUsage % numVars) << " = ";
    } else {
      p << "\nscore_part_" << (varUsage % numVars) << " += ";
    }
    p << "weights.at(" << uniVal << ");"
      << " // perceptron op";

    if (pat.usage != 1) {
      p << "\npatterns.at(" << pat.index << ") = " << name << ";";
    }
    ++varUsage;
  }

  for (auto &pat : spec_.features.pattern) {
    auto usage = pat.usage;
    if ((usage & 1) != 0) {
      // used by unigrams
      continue;
    }
    p << "\n// pattern feature #" << pat.index
      << " (without unigram), usage=" << pat.usage;
    auto name = patternValue(p, pat);
    p << "\npatterns.at(" << pat.index << ") = " << name << ";";
  }

  p << "\n// bigram and trigram state";
  for (auto &ngram : spec_.features.ngram) {
    auto ns = ngram.references.size();
    if (ns == 2) {
      NgramFeatureCodegen ngramCodegen{ngram, spec_};
      ngramCodegen.makePartialObject(p);
      ngramCodegen.biStep0(p, "patterns", "t1row");
    } else if (ns == 3) {
      NgramFeatureCodegen ngramCodegen{ngram, spec_};
      ngramCodegen.makePartialObject(p);
      ngramCodegen.triStep0(p, "patterns", "t2row");
    }
  }

  p << "\n// publish perceptron value";
  p << "\nscores.at(item) = score_part_0";
  for (int var = 1; var < numVars; ++var) {
    p << " + score_part_" << var;
  }
  p << ";";
}

void InNodeComputationsCodegen::generateLoop(i::Printer &p) {
  p << "\nauto numItems = patternMatrix.numRows();";
  p << "\nconst auto weights = scorer->weights();";
  p << "\n::jumanpp::u32 mask = weights.size() - 1;";
  p << "\n"
    << JPP_TEXT(::jumanpp::core::dic::DicEntryBuffer) << " entryBuffer;";
  p << "\nauto t1state = fbuffer->t1Buf(" << numBigrams_ << ", numItems);";
  p << "\nauto t2state = fbuffer->t2Buf1(" << numTrigrams_ << ", numItems);";
  p << "\nfor (int item = 0; item < numItems; ++item) {";
  {
    i::Indent id{p, 2};
    p << "\nauto patterns = patternMatrix.row(item);";
    p << "\nauto& nodeInfo = nodeInfos.at(item);";
    p << "\nbool status = ctx->fillEntryBuffer("
      << "nodeInfo.entryPtr(), &entryBuffer);";
    p << "\nJPP_DCHECK(status);";
    p << "\nauto entry = entryBuffer.features();";
    p << "\nauto t1row = t1state.row(item);";
    p << "\nauto t2row = t2state.row(item);";
    generateLoopBody(p);
  }
  p << "\n}";
}

InNodeComputationsCodegen::InNodeComputationsCodegen(
    const spec::AnalysisSpec &spec)
    : spec_{spec} {
  numBigrams_ = 0;
  numUnigrams_ = 0;
  numTrigrams_ = 0;
  for (auto &ngram : spec_.features.ngram) {
    auto ngramSize = ngram.references.size();
    if (ngramSize == 1) {
      numUnigrams_ += 1;
    }
    if (ngramSize == 2) {
      numBigrams_ += 1;
    } else if (ngramSize == 3) {
      numTrigrams_ += 1;
    }
  }
}

}  // namespace codegen
}  // namespace core
}  // namespace jumanpp