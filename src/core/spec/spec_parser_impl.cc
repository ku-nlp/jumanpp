//
// Created by Arseny Tolmachev on 2018/05/16.
//

#include "spec_parser_impl.h"
#include "util/mmap.h"
#include "path.hpp"

namespace jumanpp {
namespace core {
namespace spec {
namespace parser {

Status SpecParserImpl::buildSpec(AnalysisSpec *result) {
  JPP_RETURN_IF_ERROR(builder_.build(result));
  return Status::Ok();
}

struct FileResource: Resource {
  util::FullyMappedFile file_;
  Status open(StringPiece name) {
    return file_.open(name, util::MMapType::ReadOnly);
  }

  Status validate() const override {
    return Status::Ok();
  }

  StringPiece data() const override {
    return file_.contents();
  }
};

Resource *SpecParserImpl::fileResourece(StringPiece name, p::position pos) {
  Pathie::Path p{name.str()};
  auto par = p.parent();
  auto theName = par.join(name.str());
  FileResource* res = builder_.alloc_->make<FileResource>();
  builder_.garbage_.push_back(res);
  auto nameString = theName.str();
  auto s = res->open(nameString);
  if (!s) {
    throw p::parse_error("failed to open file: [" + nameString + "], error: " + s.message().str(), pos);
  }
  return res;
}

}  // namespace parser
}  // namespace spec
}  // namespace core
}  // namespace jumanpp