//
// Created by Arseny Tolmachev on 2018/05/16.
//

#include "spec_parser_impl.h"


namespace jumanpp {
namespace core {
namespace spec {
namespace parser {

Status SpecParserImpl::buildSpec(AnalysisSpec *result) {
  JPP_RETURN_IF_ERROR(builder_.build(result));
  return Status::Ok();
}

} // namespace pasrser
} // namespace spec
} // namespace core
} // namespace jumanpp