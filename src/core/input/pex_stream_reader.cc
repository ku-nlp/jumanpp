//
// Created by Arseny Tolmachev on 2018/01/19.
//

#include <memory>

#include "pex_stream_reader.h"
#include "core/analysis/score_plugin.h"
#include "core/input/partial_example.h"
#include "core/input/partial_example_io.h"

namespace jumanpp {
namespace core {
namespace input {

struct PexStreamReaderImpl : public analysis::ScorePlugin {
  std::shared_ptr<TrainFieldsIndex> tfi_;
  PartialExample example_;
  PartialExampleReader reader_;
  std::string buffer_;
  std::string temp_;

  bool updateScore(const analysis::Lattice *l,
                   const analysis::ConnectionPtr &ptr,
                   float *score) const override {
    if (!example_.doesNodeMatch(l, ptr.boundary, ptr.right)) {
      *score -= 10000.f;
      return true;
    }
    return false;
  }
};

Status PexStreamReader::readExample(std::istream *stream) {
  auto &tmp = impl_->temp_;
  auto &buf = impl_->buffer_;
  buf.clear();
  while (std::getline(*stream, tmp)) {
    if (tmp.size() == 0) {
      break;
    }
    buf.append(tmp);
    buf.push_back('\n');
  }
  JPP_RETURN_IF_ERROR(impl_->reader_.setData(buf));
  bool eof = false;
  JPP_RETURN_IF_ERROR(impl_->reader_.readExample(&impl_->example_, &eof));
  if (!eof) {
    return Status::InvalidState() << "Could not parse full example: [\n"
                                  << buf << "\n]";
  }
  return Status::Ok();
}

Status PexStreamReader::analyzeWith(analysis::Analyzer *an) {
  auto surface = impl_->example_.surface();
  auto plugin = impl_.get();
  JPP_RETURN_IF_ERROR(an->analyze(surface, plugin));
  return Status::Ok();
}

StringPiece PexStreamReader::comment() {
  return impl_->example_.exampleInfo().comment;
}

PexStreamReader::PexStreamReader() { impl_.reset(new PexStreamReaderImpl); }

Status PexStreamReader::initialize(const CoreHolder &core, char32_t noBreak) {
  impl_->tfi_ = std::make_shared<TrainFieldsIndex>();
  JPP_RETURN_IF_ERROR(impl_->tfi_->initialize(core));
  JPP_RETURN_IF_ERROR(impl_->reader_.initialize(impl_->tfi_.get(), noBreak));
  return Status::Ok();
}

Status PexStreamReader::initialize(const PexStreamReader &other) {
  impl_->tfi_ = other.impl_->tfi_; //have a copy of field index (this is fast)
  JPP_RETURN_IF_ERROR(impl_->reader_.initialize(impl_->tfi_.get(), other.impl_->reader_.noBreakToken()));
  return Status::Ok();
}

PexStreamReader::~PexStreamReader() = default;
}  // namespace input
}  // namespace core
}  // namespace jumanpp