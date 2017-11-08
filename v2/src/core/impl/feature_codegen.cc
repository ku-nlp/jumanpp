//
// Created by Arseny Tolmachev on 2017/05/26.
//

#include "feature_codegen.h"
#include <fstream>
#include "core/impl/feature_impl_combine.h"
#include "core/impl/feature_impl_ngram_partial.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace features {
namespace codegen {

Status StaticFeatureCodegen::generateAndWrite() {
  std::string headerName{config_.baseDirectory};
  headerName += '/';
  headerName += config_.filename;
  headerName += ".h";

  JPP_RETURN_IF_ERROR(writeHeader(headerName));

  std::string souceName{config_.baseDirectory};
  souceName += '/';
  souceName += config_.filename;
  souceName += ".cc";

  JPP_RETURN_IF_ERROR(writeSource(souceName));

  return Status::Ok();
}

namespace i = jumanpp::util::io;

Status StaticFeatureCodegen::writeHeader(const std::string& filename) {
  try {
    std::ofstream ofs(filename);

    util::io::Printer printer;

    ofs << "#include \"core/features_api.h\"\n\n"
        << "namespace jumanpp_generated {\n"
        << "class " << config_.className
        << ": public jumanpp::core::features::StaticFeatureFactory {\n"
        << JPP_TEXT(jumanpp::core::features::NgramFeatureApply*)
        << "ngram() const override;\n"
        << JPP_TEXT(jumanpp::core::features::PartialNgramFeatureApply*)
        << "ngramPartial() const override;\n";
#if 0
        ofs << JPP_TEXT(jumanpp::core::features::PatternFeatureApply*)
        << "pattern() const override;\n";
#endif
    ofs << "};\n"
        << "} //namespace jumanpp_generated\n";

  } catch (std::exception& e) {
    return Status::InvalidState()
           << "failed to generate " << filename << ": " << e.what();
  }

  return Status::Ok();
}

bool outputNgramFeatures(i::Printer& p, StringPiece name,
                         features::impl::NgramDynamicFeatureApply* all) {
  util::codegen::MethodBody mb{};
  auto status = all->emitCode(&mb);
  if (!status) {
    LOG_WARN() << "failed to generate ngramFeatures for " << name << ": "
               << status;
    return false;
  }

  p << "class " << name << " final : public "
    << JPP_TEXT(::jumanpp::core::features::impl::NgramFeatureApplyImpl) << " < "
    << name << " > {\n"
    << "public :";
  {
    i::Indent id{p, 2};
    p << "\ninline void apply(jumanpp::util::MutableArraySlice<jumanpp::u32> "
         "result,\n"
         "                     const jumanpp::util::ArraySlice<jumanpp::u64> "
         "&t2,\n"
         "                     const jumanpp::util::ArraySlice<jumanpp::u64> "
         "&t1,\n"
         "                     const jumanpp::util::ArraySlice<jumanpp::u64> "
         "&t0) const noexcept {\n";

    {
      i::Indent id{p, 2};
      mb.render(p);
    }
    p << "\n} // void apply";
  }
  p << "\n}; // class " << name << "\n";
  return true;
}

bool outputPartialNgramFeatures(
    i::Printer& p, StringPiece name,
    features::impl::PartialNgramDynamicFeatureApply* png) {
  p << "class " << name << " final : "
    << "public "
    << JPP_TEXT(
           ::jumanpp::core::features::impl::PartialNgramFeatureApplyImpl) "< "
    << name << " > {\n"
    << "public:";
  {
    i::Indent id{p, 2};
    p << "\n";
    JPP_RET_CHECK(png->outputClassBody(p));
  }
  p << "\n}; // class " << name << "\n";
  return true;
}

#if 0
bool outputPatternFeatures(
    i::Printer& p, StringPiece name,
    features::impl::PatternDynamicApplyImpl* patternImpl) {
  p << "class " << name << " final : "
    << "public "
    << JPP_TEXT(::jumanpp::core::features::impl::PatternFeatureApplyImpl) "< "
    << name << " > {\n"
    << "public:";
  {
    i::Indent id{p, 2};
    p << "\n";
    JPP_RET_CHECK(patternImpl->printClassBody(p));
  }
  p << "\n}; // class " << name << "\n";
  return true;
}
#endif

Status StaticFeatureCodegen::writeSource(const std::string& filename) {
  util::io::Printer p;

  p << "#include \"core/impl/feature_impl_combine.h\"\n";
  p << "#include \"core/impl/feature_impl_ngram_partial.h\"\n";
  p << "#include \"" << config_.filename << ".h\"\n\n";
  p << "namespace jumanpp_generated {\n";
  p << "namespace {\n";

  std::string ngramName{"NgramFeatureStaticApply_"};
  ngramName += config_.className;
  features::impl::NgramDynamicFeatureApply ndfa;
  features::impl::PartialNgramDynamicFeatureApply ngramPart;
  for (auto& n : spec_.features.ngram) {
    JPP_RETURN_IF_ERROR(ndfa.addChild(n));
    JPP_RETURN_IF_ERROR(ngramPart.addChild(n));
  }

  bool ngramOk = outputNgramFeatures(p, ngramName, &ndfa);
  std::string partNgramName{"PartNgramFeatureStaticApply_"};
  partNgramName += config_.className;
  bool partNgramOk = outputPartialNgramFeatures(p, partNgramName, &ngramPart);
  std::string patternName{"PatternFeatureStaticApply_"};
  patternName += config_.className;
  bool patternOk = false;
  // outputPatternFeatures(p, patternName, features.patternDynamic.get());

  p << "\n} //anon namespace\n";

  p << "\n"
    << JPP_TEXT(jumanpp::core::features::NgramFeatureApply*)
    << config_.className << "::"
    << "ngram() const {";
  {
    i::Indent io{p, 2};
    p << "\nreturn ";
    if (ngramOk) {
      p << "new " << ngramName << "{};";
    } else {
      p << "nullptr;";
    }
  }
  p << "\n}";

  p << "\n"
    << JPP_TEXT(jumanpp::core::features::PartialNgramFeatureApply*)
    << config_.className << "::"
    << "ngramPartial() const {";
  {
    i::Indent io{p, 2};
    p << "\nreturn ";
    if (partNgramOk) {
      p << "new " << partNgramName << "{};";
    } else {
      p << "nullptr;";
    }
  }
  p << "\n}";

#if 0

  p << "\n"
    << JPP_TEXT(jumanpp::core::features::PatternFeatureApply*)
    << config_.className << "::"
    << "pattern() const {";
  {
    i::Indent io{p, 2};
    p << "\nreturn ";
    if (patternOk) {
      p << "new " << patternName << "{};";
    } else {
      p << "nullptr;";
    }
  }
  p << "\n}";
#endif

  p << "\n} //jumanpp_generated namespace";

  try {
    std::ofstream ofs(filename);
    ofs << p.result();
  } catch (std::exception& e) {
    return Status::InvalidState()
           << "failed to generate " << filename << ": " << e.what();
  }
  return Status::Ok();
}

StaticFeatureCodegen::StaticFeatureCodegen(const FeatureCodegenConfig& config_,
                                           const spec::AnalysisSpec& spec)
    : config_(config_), spec_{spec} {}

}  // namespace codegen
}  // namespace features
}  // namespace core
}  // namespace jumanpp