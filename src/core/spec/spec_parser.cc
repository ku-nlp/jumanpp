//
// Created by Arseny Tolmachev on 2018/06/08.
//

#include "spec_parser.h"
#include "spec_parser_impl.h"
#include "util/mmap.h"

#include <pegtl/memory_input.hpp>
#include <pegtl/parse.hpp>

namespace jumanpp {
namespace core {
namespace spec {

namespace p = ::tao::jumanpp_pegtl;

Status parseFromFile(StringPiece name, AnalysisSpec* result) {
  util::FullyMappedFile specFile;
  JPP_RETURN_IF_ERROR(specFile.open(name, util::MMapType::ReadOnly));

  parser::SpecParserImpl impl{name};

  auto data = specFile.contents();
  std::string filenameString = name.str();
  p::memory_input<> input{data.char_begin(), data.char_end(), filenameString};
  try {
    p::parse<parser::full_spec, parser::SpecAction>(input, impl);
  } catch (p::parse_error& e) {
    return JPPS_INVALID_PARAMETER << "failed to parse spec:\n" << e.what();
  }

  JPP_RETURN_IF_ERROR(impl.builder_.build(result));

  return Status::Ok();
}

}  // namespace spec
}  // namespace core
}  // namespace jumanpp