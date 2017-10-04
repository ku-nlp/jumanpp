//
// Created by Arseny Tolmachev on 2017/05/26.
//

#include "feature_codegen.h"
#include <fstream>
#include "core/impl/feature_impl_combine.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace features {
namespace codegen {

Status StaticFeatureCodegen::generateAndWrite(const FeatureHolder& features) {
  std::string headerName{config_.baseDirectory};
  headerName += '/';
  headerName += config_.filename;
  headerName += ".h";

  JPP_RETURN_IF_ERROR(writeHeader(headerName));

  std::string souceName{config_.baseDirectory};
  souceName += '/';
  souceName += config_.filename;
  souceName += ".cc";

  JPP_RETURN_IF_ERROR(writeSource(souceName, features));

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
        << "jumanpp::core::features::NgramFeatureApply* ngram() const "
           "override;\n"
        << "};\n"
        << "} //namespace jumanpp_generated\n";

  } catch (std::exception& e) {
    return Status::InvalidState()
           << "failed to generate " << filename << ": " << e.what();
  }

  return Status::Ok();
}

bool outputNgramFeatures(i::Printer& p, StringPiece name,
                         NgramFeatureApply* ngr) {
  auto all = dynamic_cast<features::impl::NgramDynamicFeatureApply*>(ngr);
  if (!all) {
    return false;
  }

  util::codegen::MethodBody mb{};
  auto status = all->emitCode(&mb);
  if (!status) {
    LOG_WARN() << "failed to generate ngramFeatures for " << name << ": "
               << status;
    return false;
  }

  p << "class " << name
    << " final : public jumanpp::core::features::impl::NgramFeatureApplyImpl< "
    << name << " > {\n"
    << "public:";
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
  p << "\n}; // class " << name;
  return true;
}

Status StaticFeatureCodegen::writeSource(const std::string& filename,
                                         const FeatureHolder& features) {
  util::io::Printer p;

  p << "#include \"util/seahash.h\"\n";
  p << "#include \"core/impl/feature_impl_combine.h\"\n";
  p << "#include \"" << config_.filename << ".h\"\n\n";
  p << "namespace jumanpp_generated {\n";
  p << "namespace {\n";

  std::string ngramName{"NgramFeatureStaticApply_"};
  ngramName += config_.className;
  bool ngramOk = outputNgramFeatures(p, ngramName, features.ngramDynamic.get());

  p << "\n} //anon namespace\n";

  p << "jumanpp::core::features::NgramFeatureApply* " << config_.className
    << "::"
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

StaticFeatureCodegen::StaticFeatureCodegen(const FeatureCodegenConfig& config_)
    : config_(config_) {}

}  // namespace codegen
}  // namespace features
}  // namespace core
}  // namespace jumanpp